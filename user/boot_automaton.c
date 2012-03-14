#include <automaton.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <callback_queue.h>
#include <description.h>
#include <dymem.h>
#include "cpio.h"
#include "vfs_msg.h"
#include "argv.h"

/*
  The Boot Automaton
  ==================
  The goal of the boot automaton is to create an environment that allows automata to load other automata from a store.
  The boot automaton receives a buffer containing a cpio archive containing programs for a vfs, tmpfs, and shell from which is tries to create automata.
  The buffer supplied to the boot automaton is passed to the tmpfs automaton.
  Any failures cause the boot automaton to exit.

  TODO:  Should we subscribe to the automata to make sure they don't die?

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define VFS_REQUEST_NO 1
#define VFS_RESPONSE_NO 2
#define DESTROY_BUFFERS_NO 3

/* Name to register the VFS under. */
#define VFS_NAME "vfs"

/* Paths in the cpio archive. */
#define VFS_PATH "bin/vfs"
#define TMPFS_PATH "bin/tmpfs"

/* Mount the tmpfs here. */
#define ROOT_PATH "/"

/* Create a shell located here with this argument. */
#define SHELL_PATH "/bin/jsh"
#define SHELL_CMDLINE "/scr/start.jsh"

/* Initialization flag. */
static bool initialized = false;

/* Queue contains requests for the vfs. */
static bd_t vfs_request_bda = -1;
static bd_t vfs_request_bdb = -1;
static vfs_request_queue_t vfs_request_queue;

/* Callbacks to execute from the vfs. */
static callback_queue_t vfs_response_queue;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    vfs_request_bda = buffer_create (1);
    if (vfs_request_bda == -1) {
      syslog ("boot_automaton: error: Could not create vfs request buffer");
      exit ();
    }
    vfs_request_bdb = buffer_create (0);
    if (vfs_request_bdb == -1) {
      syslog ("boot_automaton: error: Could not create vfs request buffer");
      exit ();
    }

    vfs_request_queue_init (&vfs_request_queue);
    callback_queue_init (&vfs_response_queue);
  }
}

static void
schedule (void);

static void
end_input_action (bd_t bda,
		  bd_t bdb)
{
  if (bda != -1) {
    buffer_destroy (bda);
  }
  if (bdb != -1) {
    buffer_destroy (bdb);
  }
  schedule ();
  scheduler_finish (false, -1, -1);
}

static void
end_output_action (bool output_fired,
		   bd_t bda,
		   bd_t bdb)
{
  schedule ();
  scheduler_finish (output_fired, bda, bdb);
}

static void
readfile_callback (void* data,
		   bd_t bda,
		   bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) != 0) {
    syslog ("boot_automaton: error: vfs provided bad readfile response");
    exit ();
  }

  if (error != VFS_SUCCESS) {
    syslog ("boot_automaton: error: Could not read " SHELL_PATH);
    exit ();
  }

  bd_t bd1;
  bd_t bd2;

  argv_t argv;
  if (argv_initw (&argv, &bd1, &bd2) != 0) {
    syslog ("boot_automaton: error: Could not initialize argv");
    exit ();
  }

  if (argv_append (&argv, SHELL_PATH, strlen (SHELL_PATH) + 1) != 0) {
    syslog ("boot_automaton: error: Could not write to argv");
    exit ();
  }

  if (argv_append (&argv, SHELL_CMDLINE, strlen (SHELL_CMDLINE) + 1) != 0) {
    syslog ("boot_automaton: error: Could not write to argv");
    exit ();
  }

  if (create (bdb, size, bd1, bd2, 0, 0, true) == -1) {
    syslog ("boot_automaton: error: Could not create " SHELL_PATH);
    exit ();
  }

  /* The bdb buffer will be destroyed by the input action that calls this callback. */
}

static void
mount_callback (void* data,
		bd_t bda,
		bd_t bdb)
{
  vfs_error_t error;
  if (read_vfs_mount_response (bda, bdb, &error) != 0) {
    syslog ("boot_automaton: error: vfs provided bad mount response");
    exit ();
  }

  if (error != VFS_SUCCESS) {
    syslog ("boot_automaton: error: mounting tmpfs on " ROOT_PATH " failed");
    exit ();
  }

  /* Request the shell. */
  vfs_request_queue_push_readfile (&vfs_request_queue, SHELL_PATH);
  callback_queue_push (&vfs_response_queue, readfile_callback, 0);
}

/* init
   ----
   Init receives a buffer containing a cpio archive.
   It looks for the files registry, vfs, tmpfs, and init and creates automata from them except for init.
   It passes the cpio archive to tmpfs.
   
   Post: mount_state == MOUNT && tmpfs != -1 && init_file != 0
 */
BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t boot_aid, bd_t bda, bd_t bdb)
{
  initialize ();

  cpio_archive_t archive;
  if (cpio_archive_init (&archive, bda) != 0) {
    syslog ("boot_automaton: error: Could not initialize cpio archive");
    exit ();
  }

  cpio_file_t* vfs_file = 0;
  cpio_file_t* tmpfs_file = 0;
  cpio_file_t* file;
  while ((file = cpio_archive_next_file (&archive)) != 0) {
    if (strcmp (file->name, VFS_PATH) == 0) {
      vfs_file = file;
    }
    else if (strcmp (file->name, TMPFS_PATH) == 0) {
      tmpfs_file = file;
    }
    else {
      /* Destroy the file if we don't need it. */
      cpio_file_destroy (file);
    }
  }
  
  /* Create the vfs. */
  if (vfs_file == 0) {
    syslog ("boot_automaton: error: No vfs file");
    exit ();
  }
  aid_t vfs = create (vfs_file->bd, vfs_file->size, -1, -1, VFS_NAME, strlen (VFS_NAME) + 1, false);
  cpio_file_destroy (vfs_file);
  if (vfs == -1) {
    syslog ("boot_automaton: error: Could not create vfs");
    exit ();
  }

  /* Bind to the vfs so we can mount the tmpfs. */
  description_t vfs_description;
  if (description_init (&vfs_description, vfs) != 0) {
    syslog ("boot_automaton: error: Could not describe vfs");
    exit ();
  }

  /* If these actions don't exist, then attempts to bind below will fail. */
  const ano_t vfs_request = description_name_to_number (&vfs_description, VFS_REQUEST_NAME, strlen (VFS_REQUEST_NAME) + 1);
  const ano_t vfs_response = description_name_to_number (&vfs_description, VFS_RESPONSE_NAME, strlen (VFS_RESPONSE_NAME) + 1);

  description_fini (&vfs_description);

  /* We bind the response first so they don't get lost. */
  if (bind (vfs, vfs_response, 0, boot_aid, VFS_RESPONSE_NO, 0) == -1 ||
      bind (boot_aid, VFS_REQUEST_NO, 0, vfs, vfs_request, 0) == -1) {
    syslog ("boot_automaton: error: Could not bind to vfs");
    exit ();
  }

  /* Create the tmpfs. */
  if (tmpfs_file == 0) {
    syslog ("boot_automaton: error: No tmpfs file\n");
    exit ();
  }
  /* Note:  We pass the buffer containing the cpio archive. */
  aid_t tmpfs = create (tmpfs_file->bd, tmpfs_file->size, bda, -1, 0, 0, false);
  cpio_file_destroy (tmpfs_file);
  if (tmpfs == -1) {
    syslog ("boot_automaton: error: Could not create tmpfs");
    exit ();
  }

  /* Mount tmpfs on ROOT_PATH. */
  vfs_request_queue_push_mount (&vfs_request_queue, tmpfs, ROOT_PATH);
  callback_queue_push (&vfs_response_queue, mount_callback, 0);

  end_input_action (bda, bdb);
}

/* vfs_request
   -------------
   Send a request to the vfs.

   Pre:  vfs_bd != -1
   Post: vfs_bd == -1
 */
static bool
vfs_request_precondition (void)
{
  return !vfs_request_queue_empty (&vfs_request_queue);
}

BEGIN_OUTPUT (NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, int param)
{
  initialize ();
  scheduler_remove (VFS_REQUEST_NO, param);

  if (vfs_request_precondition ()) {
    if (vfs_request_queue_pop_to_buffer (&vfs_request_queue, vfs_request_bda, vfs_request_bdb) != 0) {
      syslog ("boot_automaton: error: Could not write to output buffer");
      exit ();
    }
    end_output_action (true, vfs_request_bda, vfs_request_bdb);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

/* vfs_response
   --------------
   Extract the aid of the vfs from the response from the registry.

   Post: ???
 */
BEGIN_INPUT (NO_PARAMETER, VFS_RESPONSE_NO, "", "", vfs_response, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (callback_queue_empty (&vfs_response_queue)) {
    syslog ("boot_automaton: error: vfs produced spurious response");
    exit ();
  }

  const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
  callback_t callback = callback_queue_item_callback (item);
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&vfs_response_queue);

  callback (data, bda, bdb);

  end_input_action (bda, bdb);
}

static void
schedule (void)
{
  if (vfs_request_precondition ()) {
    scheduler_add (VFS_REQUEST_NO, 0);
  }
}
