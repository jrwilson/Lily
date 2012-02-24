#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <buffer_queue.h>
#include <description.h>
#include "cpio.h"
#include "vfs_msg.h"

/*
  The Boot Automaton
  ==================
  The goal of the boot automaton is to create an environment that allows automata to load other automata from a store.
  The boot automaton receives a buffer containing a cpio archive.
  The boot automaton looks for the following file names:  registry, vfs, tmpfs, and init.
  The boot automaton then tries to create automata based on the contents of the registry, vfs, tmpfs, and init files.
  The buffer supplied to the boot automaton is passed to the tmpfs automaton.
  Any failures cause the boot automaton to exit.

  TODO:  Should we subscribe to the automata to make sure they don't die?

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define VFS_REQUEST_NO 1
#define VFS_RESPONSE_NO 2
#define DESTROY_BUFFERS_NO 3

#define ROOT_PATH "/"

/* Initialization flag. */
static bool initialized = false;

/* File containing the init automaton. */
static cpio_file_t* init_file = 0;

/* Buffer containing the mount command. */
static bd_t mount_bd = -1;
static size_t mount_bd_size;

/* Queue of buffers that need to be destroyed. */
static buffer_queue_t destroy_queue;

static void
end_action (bool output_fired,
	    bd_t bd,
	    size_t bd_size);

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    buffer_queue_init (&destroy_queue);
  }
}

static void
ssyslog (const char* msg)
{
  syslog (msg, strlen (msg));
}

/* init
   ----
   Init receives a buffer containing a cpio archive.
   It looks for the files registry, vfs, tmpfs, and init and creates automata from them except for init.
   It passes the cpio archive to tmpfs.
   
   Post: mount_state == MOUNT && tmpfs != -1 && init_file != 0
 */
BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t boot_aid, bd_t bd, size_t bd_size)
{
  initialize ();

  cpio_archive_t archive;
  if (cpio_archive_init (&archive, bd, bd_size) == -1) {
    ssyslog ("boot_automaton: error: Could not initialize cpio archive\n");
    exit ();
  }

  cpio_file_t* registry_file = 0;
  cpio_file_t* vfs_file = 0;
  cpio_file_t* tmpfs_file = 0;
  cpio_file_t* file;
  while ((file = cpio_archive_next_file (&archive)) != 0) {
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

  /* Create the registry first so the other automata can use it. */
  if (registry_file == 0) {
    ssyslog ("boot_automaton: error: No registry file\n");
    exit ();
  }
  aid_t registry = create (registry_file->bd, true, -1);
  cpio_file_destroy (registry_file);
  registry_file = 0;
  if (registry == -1) {
    ssyslog ("boot_automaton: error: Could not create the registry\n");
    exit ();
  }
  if (set_registry (registry) == -1) {
    ssyslog ("boot_automaton: error: Could not set the registry\n");
    exit ();
  }
  
  /* Create the vfs. */
  if (vfs_file == 0) {
    ssyslog ("boot_automaton: error: No vfs file\n");
    exit ();
  }
  aid_t vfs = create (vfs_file->bd, true, -1);
  cpio_file_destroy (vfs_file);
  if (vfs == -1) {
    ssyslog ("boot_automaton: error: Could not create vfs\n");
    exit ();
  }

  /* Bind to the vfs so we can mount the tmpfs. */
  description_t vfs_description;
  if (description_init (&vfs_description, vfs) == -1) {
    ssyslog ("boot_automaton: error: Could not describe vfs\n");
    exit ();
  }

  /* If these actions don't exist, then attempts to bind below will fail. */
  const ano_t vfs_request = description_name_to_number (&vfs_description, VFS_REQUEST_NAME);
  const ano_t vfs_response = description_name_to_number (&vfs_description, VFS_RESPONSE_NAME);

  description_fini (&vfs_description);

  /* We bind the response first so they don't get lost. */
  if (bind (vfs, vfs_response, 0, boot_aid, VFS_RESPONSE_NO, 0) == -1 ||
      bind (boot_aid, VFS_REQUEST_NO, 0, vfs, vfs_request, 0) == -1) {
    ssyslog ("boot_automaton: error: Couldn't bind to vfs\n");
    exit ();
  }

  /* Create the tmpfs. */
  if (tmpfs_file == 0) {
    ssyslog ("boot_automaton: error: No tmpfs file\n");
    exit ();
  }
  /* Note:  We pass the buffer containing the cpio archive. */
  aid_t tmpfs = create (tmpfs_file->bd, true, bd);
  cpio_file_destroy (tmpfs_file);
  if (tmpfs == -1) {
    ssyslog ("boot_automaton: error: Could not create tmpfs\n");
    exit ();
  }

  /* Mount tmpfs on ROOT_PATH. */
  mount_bd_size = 0;
  mount_bd = write_vfs_mount_request (tmpfs, ROOT_PATH, &mount_bd_size);

  /* Create the init automaton. */
  if (init_file == 0) {
    ssyslog ("boot_automaton: error: No init file\n");
    exit ();
  }

  end_action (false, bd, bd_size);
}

/* vfs_request
   -----------
   Sent a request to mount tmpfs as the root file system of vfs.

   Pre:  mount_bd != -1
   Post: mount_bd == -1
 */
static bool
vfs_request_precondition (void)
{
  return mount_bd != -1;
}

BEGIN_OUTPUT(NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, int param, size_t bc)
{
  initialize ();
  scheduler_remove (VFS_REQUEST_NO, param);
  
  if (vfs_request_precondition ()) {
    ssyslog ("boot_automaton: vfs_request\n");
    bd_t bd = mount_bd;
    size_t bd_size = mount_bd_size;

    mount_bd = -1;
    mount_bd_size = 0;

    end_action (true, bd, bd_size);
  }
  else {
    end_action (false, -1, 0);
  }
}

/* vfs_response
   ------------
   Receive a response from the vfs_automaton.

   Post: exit on error, otherwise, create the init automaton
 */
BEGIN_INPUT (NO_PARAMETER, VFS_RESPONSE_NO, "", "", vfs_response, int param, bd_t bd, size_t bd_size)
{
  ssyslog ("boot_automaton: vfs_response\n");
  initialize ();

  vfs_error_t error;
  if (read_vfs_mount_response (bd, bd_size, &error) == -1) {
    ssyslog ("boot_automaton: error: vfs provide bad mount response\n");
    exit ();
  }

  if (error != VFS_SUCCESS) {
    ssyslog ("boot_automaton: error: mounting root file system failed\n");
    exit ();
  }

  /* Now that the root file system is mounted, create the init automaton. */
  aid_t init = create (init_file->bd, true, -1);
  cpio_file_destroy (init_file);
  if (init == -1) {
    ssyslog ("boot_automaton: error: Could not create init\n");
    exit ();
  }

  end_action (false, bd, bd_size);
}

/* destroy_buffers
   ---------------
   Destroys all of the buffers in destroy_queue.
   This is useful for output actions that need to destroy the buffer *after* the output has fired.
   To schedule a buffer for destruction, just add it to destroy_queue.

   Pre:  Destroy queue is not empty.
   Post: Destroy queue is empty.
 */
static bool
destroy_buffers_precondition (void)
{
  return !buffer_queue_empty (&destroy_queue);
}

BEGIN_INTERNAL (NO_PARAMETER, DESTROY_BUFFERS_NO, "", "", destroy_buffers, int param)
{
  initialize ();
  scheduler_remove (DESTROY_BUFFERS_NO, param);

  if (destroy_buffers_precondition ()) {
    while (!buffer_queue_empty (&destroy_queue)) {
      buffer_destroy (buffer_queue_item_bd (buffer_queue_front (&destroy_queue)));
      buffer_queue_pop (&destroy_queue);
    }
  }

  end_action (false, -1, 0);
}

/* end_action is a helper function for terminating actions.
   If the buffer is not -1, it schedules it to be destroyed.
   end_action schedules local actions and calls scheduler_finish to finish the action.
*/
static void
end_action (bool output_fired,
	    bd_t bd,
	    size_t bd_size)
{
  if (bd != -1) {
    buffer_queue_push (&destroy_queue, 0, bd, bd_size);
  }

  if (vfs_request_precondition ()) {
    scheduler_add (VFS_REQUEST_NO, 0);
  }
  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }

  scheduler_finish (output_fired, bd);
}
