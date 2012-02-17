#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include "cpio.h"
#include "vfs.h"

/*
  The Boot Automaton
  ==================
  The goal of the boot automaton is to create an environment that allows automata to load other automata from a store.
  The boot automaton receives a buffer containing a cpio archive.
  The boot automaton looks for the following file names:  registry, vfs, tmpfs, and init.
  The boot automaton then tries to create automata based on the contents of the registry, vfs, tmpfs, and init files.
  The buffer supplied to the boot automaton is passed to the tmpfs automaton.
  If one of the files does not exist in the archive, the boot automaton prints a warning and continues.
  Similarly, a warning is emitted if an automaton cannot be created.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define VFS_REQUEST_NO 1
#define VFS_RESPONSE_NO 2

#define ROOT_PATH "/"

typedef enum {
  START,
  MOUNT,
  SENT,
} mount_state_t;
static mount_state_t mount_state = START;
static aid_t tmpfs_aid = -1;

static void
schedule (void);

static void
create_registry (cpio_file_t* registry_file)
{
  if (registry_file == 0) {
    const char* s = "boot_automaton: warning: No registry\n";
    syslog (s, strlen (s));
    return;
  }

  aid_t registry = create (registry_file->bd, registry_file->size, true, -1);
  if (registry == -1) {
    const char* s = "boot_automaton: warning: Could not create registry\n";
    syslog (s, strlen (s));
    return;
  }

  if (set_registry (registry) == -1) {
    const char* s = "boot_automaton: warning: Could not set registry\n";
    syslog (s, strlen (s));
    return;
  }
}

static void
create_vfs (cpio_file_t* vfs_file,
	    aid_t boot_aid)
{
  aid_t vfs_aid = create (vfs_file->bd, vfs_file->size, true, -1);
  if (vfs_aid == -1) {
    const char* s = "boot_automaton: warning: Could not create vfs\n";
    syslog (s, strlen (s));
    return;
  }

  /* Bind to the VFS. */

  /* Get its description. */
  bd_t bd = describe (vfs_aid);
  if (bd == -1) {
    const char* s = "boot_automaton: warning: no vfs description\n";
    syslog (s, strlen (s));
    return;
  }

  size_t bd_size = buffer_size (bd);

  /* Map the description. */
  void* ptr = buffer_map (bd);
  if (ptr == 0) {
    const char* s = "boot_automaton: warning: could not map vfs description\n";
    syslog (s, strlen (s));
    return;
  }

  const ano_t request = action_name_to_number (bd, bd_size, ptr, VFS_REQUEST_NAME);
  const ano_t response = action_name_to_number (bd, bd_size, ptr, VFS_RESPONSE_NAME);

  /* Dispense with the buffer. */
  buffer_destroy (bd);

  if (bind (boot_aid, VFS_REQUEST_NO, 0, vfs_aid, request, 0) == -1 ||
      bind (vfs_aid, response, 0, boot_aid, VFS_RESPONSE_NO, 0) == -1) {
    const char* s = "boot_automaton: warning: couldn't bind to vfs\n";
    syslog (s, strlen (s));
    return;
  }

  mount_state = MOUNT;
}

static void
create_tmpfs (cpio_file_t* tmpfs_file,
	      bd_t bd)
{
  /* Note that we pass along the data (bd) to the tmpfs automaton. */
  tmpfs_aid = create (tmpfs_file->bd, tmpfs_file->size, true, bd);
  if (tmpfs_aid == -1) {
    const char* s = "boot_automaton: warning: Could not create tmpfs\n";
    syslog (s, strlen (s));
    return;
  }
}

static void
create_init (cpio_file_t* init_file)
{
  if (create (init_file->bd, init_file->size, true, -1) == -1) {
    const char* s = "boot_automaton: warning: Could not create init\n";
    syslog (s, strlen (s));
    return;
  }
}

void
init (aid_t boot_aid,
      bd_t bd,
      size_t bd_size,
      void* ptr)
{
  if (bd == -1) {
    const char* s = "boot_automaton: error: No buffer\n";
    syslog (s, strlen (s));
    exit ();
  }

  if (bd_size == 0) {
    const char* s = "boot_automaton: error: Buffer is empty\n";
    syslog (s, strlen (s));
    exit ();
  }

  if (ptr == 0) {
    const char* s = "boot_automaton: error: Buffer could not be mapped\n";
    syslog (s, strlen (s));
    exit ();
  }

  /* Parse the cpio archive looking for files that we need. */
  buffer_file_t bf;
  buffer_file_open (&bf, bd, bd_size, ptr, false);

  cpio_file_t* registry_file = 0;
  cpio_file_t* vfs_file = 0;
  cpio_file_t* tmpfs_file = 0;
  cpio_file_t* init_file = 0;
  cpio_file_t* file;
  while ((file = parse_cpio (&bf)) != 0) {
    if (strcmp (file->name, "registry") == 0) {
      registry_file = file;
    }
    else if (strcmp (file->name, "vfs") == 0) {
      vfs_file = file;
    }
    else if (strcmp (file->name, "tmpfs") == 0) {
      tmpfs_file = file;
    }
    else if (strcmp (file->name, "init") == 0) {
      init_file = file;
    }
    else {
      /* Destroy the file if we don't need it. */
      cpio_file_destroy (file);
    }
  }

  /*
    Create the automata.
    In general, we create the automaton and then destroy the cpio_file_t* or we emit a warning that the automaton was not provided.
  */
  if (registry_file != 0) {
    create_registry (registry_file);
    cpio_file_destroy (registry_file);
  }
  else {
    const char* s = "boot_automaton: warning: No registry\n";
    syslog (s, strlen (s));
  }

  if (vfs_file != 0) {
    create_vfs (vfs_file, boot_aid);
    cpio_file_destroy (vfs_file);
  }
  else {
    const char* s = "boot_automaton: warning: No vfs\n";
    syslog (s, strlen (s));
  }

  if (tmpfs_file != 0) {
    create_tmpfs (tmpfs_file, bd);
    cpio_file_destroy (tmpfs_file);
  }
  else {
    const char* s = "boot_automaton: warning: No tmpfs\n";
    syslog (s, strlen (s));
  }

  if (init_file != 0) {
    create_init (init_file);
    cpio_file_destroy (init_file);
  }
  else {
    const char* s = "boot_automaton: warning: No init\n";
    syslog (s, strlen (s));
  }

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, PARAMETER, AUTO_MAP, init, INIT, "", "");

static bool
vfs_request_precondition (void)
{
  return tmpfs_aid != -1 && mount_state == MOUNT;
}

void
vfs_request (int param,
	     size_t bc)
{
  scheduler_remove (VFS_REQUEST_NO, param);

  if (vfs_request_precondition ()) {
    /* Mount tmpfs on /. */
    buffer_file_t file;
    buffer_file_create (&file, sizeof (vfs_request_t));
    vfs_request_t r;
    r.type = VFS_MOUNT;
    r.u.mount.aid = tmpfs_aid;
    size_t s = strlen (ROOT_PATH) + 1;
    r.u.mount.path_size = s;
    buffer_file_write (&file, &r, sizeof (vfs_request_t));
    buffer_file_write (&file, ROOT_PATH, s);

    bd_t bd = buffer_file_bd (&file);

    mount_state = SENT;

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, FINISH_NOOP);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, 0, vfs_request, VFS_REQUEST_NO, "", "");

static void
process_mount_response (bd_t bd,
			size_t bd_size,
			void* ptr)
{
  if (ptr == 0) {
    const char* s = "boot_automaton: warning: Could not read mount response\n";
    syslog (s, strlen (s));
    return;
  }

  buffer_file_t file;
  buffer_file_open (&file, bd, bd_size, ptr, false);
  const vfs_response_t* r = buffer_file_readp (&file, sizeof (vfs_response_t));
  if (r == 0) {
    const char* s = "boot_automaton: warning: Could not read mount response\n";
    syslog (s, strlen (s));
    return;
  }

  if (r->type != VFS_MOUNT) {
    const char* s = "boot_automaton: warning: Mount returned strange type\n";
    syslog (s, strlen (s));
    return;
  }

  switch (r->error) {
  case VFS_SUCCESS:
    /* Okay. */
    break;
  default:
    {
      const char* s = "boot_automaton: warning: Mounting tmpfs failed\n";
      syslog (s, strlen (s));
      return;
    }
    break;
  }
}

void
vfs_response (int param,
	      bd_t bd,
	      size_t bd_size,
	      void* ptr)
{
  process_mount_response (bd, bd_size, ptr);

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, AUTO_MAP, vfs_response, VFS_RESPONSE_NO, "", "");

static void
schedule (void)
{
  if (vfs_request_precondition ()) {
    scheduler_add (VFS_REQUEST_NO, 0);
  }
}
