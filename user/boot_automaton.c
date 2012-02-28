#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <buffer_queue.h>
#include <callback_queue.h>
#include <description.h>
#include "cpio.h"
#include "vfs_msg.h"
#include "argv.h"

/*
  The Boot Automaton
  ==================
  The goal of the boot automaton is to create an environment that allows automata to load other automata from a store.
  The boot automaton receives a buffer containing a cpio archive containing a registry, vfs, tmpfs, and shell from which is tries to create automata.
  The buffer supplied to the boot automaton is passed to the tmpfs automaton.
  Any failures cause the boot automaton to exit.

  TODO:  Should we subscribe to the automata to make sure they don't die?

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define VFS_REQUEST_NO 1
#define VFS_RESPONSE_NO 2
#define DESTROY_BUFFERS_NO 3

/* Paths in the cpio archive. */
#define REGISTRY_PATH "bin/registry"
#define VFS_PATH "bin/vfs"
#define TMPFS_PATH "bin/tmpfs"

/* Mount the tmpfs here. */
#define ROOT_PATH "/"

/* Create a shell located here with this argument. */
#define SHELL_PATH "/bin/jsh"
#define SHELL_CMDLINE "This is the command line!"

/* Initialization flag. */
static bool initialized = false;

/* Queue of buffers that need to be destroyed. */
static buffer_queue_t destroy_queue;

/* Messages to the vfs. */
static buffer_queue_t vfs_request_queue;

/* Callbacks to execute from the vfs. */
static callback_queue_t vfs_response_queue;

static void
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb);

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    buffer_queue_init (&destroy_queue);
    buffer_queue_init (&vfs_request_queue);
    callback_queue_init (&vfs_response_queue);
  }
}

static void
ssyslog (const char* msg)
{
  syslog (msg, strlen (msg));
}

static void
readfile_callback (void* data,
		   bd_t bda,
		   bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) == -1) {
    ssyslog ("boot_automaton: error: vfs provide bad readfile response\n");
    exit ();
  }

  if (error != VFS_SUCCESS) {
    ssyslog ("boot_automaton: error: reading " SHELL_PATH " failed \n");
    exit ();
  }

  bd_t bd1;
  bd_t bd2;

  argv_t argv;
  if (argv_initw (&argv, &bd1, &bd2) == -1) {
    ssyslog ("boot_automaton: error: could not initialize argv\n");
    exit ();
  }

  if (argv_append (&argv, SHELL_CMDLINE) == -1) {
    ssyslog ("boot_automaton: error: could not initialize argv\n");
    exit ();
  }

  if (create (bdb, size, bd1, bd2, true) == -1) {
    ssyslog ("boot_automaton: error: could not create the shell\n");
    exit ();
  }
}

static void
mount_callback (void* data,
		bd_t bda,
		bd_t bdb)
{
  vfs_error_t error;
  if (read_vfs_mount_response (bda, &error) == -1) {
    ssyslog ("boot_automaton: error: vfs provide bad mount response\n");
    exit ();
  }

  if (error != VFS_SUCCESS) {
    ssyslog ("boot_automaton: error: mounting root file system failed\n");
    exit ();
  }

  /* Request the shell. */
  {
    bd_t bd = write_vfs_readfile_request (SHELL_PATH);
    if (bd == -1) {
      ssyslog ("boot_automaton: error: Couldn't create readfile request\n");
      exit ();
    }
    buffer_queue_push (&vfs_request_queue, 0, bd, -1);
    callback_queue_push (&vfs_response_queue, readfile_callback, 0);
  }
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
  if (cpio_archive_init (&archive, bda) == -1) {
    ssyslog ("boot_automaton: error: Could not initialize cpio archive\n");
    exit ();
  }

  cpio_file_t* registry_file = 0;
  cpio_file_t* vfs_file = 0;
  cpio_file_t* tmpfs_file = 0;
  cpio_file_t* file;
  while ((file = cpio_archive_next_file (&archive)) != 0) {
    if (strcmp (file->name, REGISTRY_PATH) == 0) {
      registry_file = file;
    }
    else if (strcmp (file->name, VFS_PATH) == 0) {
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

  /* Create the registry first so the other automata can use it. */
  if (registry_file == 0) {
    ssyslog ("boot_automaton: error: No registry file\n");
    exit ();
  }
  aid_t registry = create (registry_file->bd, registry_file->size, -1, -1, true);
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
  aid_t vfs = create (vfs_file->bd, vfs_file->size, -1, -1, true);
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
  aid_t tmpfs = create (tmpfs_file->bd, tmpfs_file->size, bda, -1, true);
  cpio_file_destroy (tmpfs_file);
  if (tmpfs == -1) {
    ssyslog ("boot_automaton: error: Could not create tmpfs\n");
    exit ();
  }

  /* Mount tmpfs on ROOT_PATH. */
  {
    bd_t bd = write_vfs_mount_request (tmpfs, ROOT_PATH);
    if (bd == -1) {
      ssyslog ("boot_automaton: error: Couldn't create mount request\n");
      exit ();
    }
    buffer_queue_push (&vfs_request_queue, 0, bd, -1);
    callback_queue_push (&vfs_response_queue, mount_callback, 0);
  }

  end_action (false, bda, bdb);
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
  return !buffer_queue_empty (&vfs_request_queue);
}

BEGIN_OUTPUT (NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, int param)
{
  initialize ();
  scheduler_remove (VFS_REQUEST_NO, param);

  if (vfs_request_precondition ()) {
    ssyslog ("boot_automaton: vfs_request\n");
    const buffer_queue_item_t* item = buffer_queue_front (&vfs_request_queue);
    bd_t bda = buffer_queue_item_bda (item);
    bd_t bdb = buffer_queue_item_bdb (item);
    buffer_queue_pop (&vfs_request_queue);

    end_action (true, bda, bdb);
  }
  else {
    end_action (false, -1, -1);
  }
}

/* vfs_response
   --------------
   Extract the aid of the vfs from the response from the registry.

   Post: ???
 */
BEGIN_INPUT (NO_PARAMETER, VFS_RESPONSE_NO, "", "", vfs_response, int param, bd_t bda, bd_t bdb)
{
  ssyslog ("boot_automaton: vfs_response\n");
  initialize ();

  if (callback_queue_empty (&vfs_response_queue)) {
    ssyslog ("boot_automaton: error: vfs produced spurious response\n");
    exit ();
  }

  const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
  callback_t callback = callback_queue_item_callback (item);
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&vfs_response_queue);

  callback (data, bda, bdb);

  end_action (false, bda, bdb);
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
      bd_t bd;
      const buffer_queue_item_t* item = buffer_queue_front (&destroy_queue);
      bd = buffer_queue_item_bda (item);
      if (bd != -1) {
	buffer_destroy (bd);
      }
      bd = buffer_queue_item_bdb (item);
      if (bd != -1) {
	buffer_destroy (bd);
      }

      buffer_queue_pop (&destroy_queue);
    }
  }

  end_action (false, -1, -1);
}

/* end_action is a helper function for terminating actions.
   If the buffer is not -1, it schedules it to be destroyed.
   end_action schedules local actions and calls scheduler_finish to finish the action.
*/
static void
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb)
{
  if (bda != -1 || bdb != -1) {
    buffer_queue_push (&destroy_queue, 0, bda, bdb);
  }

  if (vfs_request_precondition ()) {
    scheduler_add (VFS_REQUEST_NO, 0);
  }
  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }

  scheduler_finish (output_fired, bda, bdb);
}
