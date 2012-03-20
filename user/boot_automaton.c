#include <automaton.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <callback_queue.h>
#include <description.h>
#include <dymem.h>
#include "cpio.h"
#include "vfs_msg.h"
#include "argv.h"
#include "syslog.h"

/*
  The Boot Automaton
  ==================
  The goal of the boot automaton is to create an environment that allows automata to load other automata from a store.
  The boot automaton receives a buffer containing a cpio archive containing programs for a vfs, tmpfs, and shell from which is tries to create automata.
  The buffer supplied to the boot automaton is passed to the tmpfs automaton.
  The boot automaton also attempts to create a logging automaton.
  Any failures cause the boot automaton to log a message and exit.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define INIT_NO 1
#define STOP_NO 2
#define SYSLOG_NO 3
#define VFS_REQUEST_NO 4
#define VFS_RESPONSE_NO 5

#define ERROR "boot_automaton: error: "
#define WARNING "boot_automaton: warning: "

/* Names for registration. */
#define VFS_NAME "vfs"

/* Paths in the cpio archive. */
#define SYSLOG_PATH "bin/syslog"
#define VFS_PATH "bin/vfs"
#define TMPFS_PATH "bin/tmpfs"

/* Mount the tmpfs here. */
#define ROOT_PATH "/"

/* Create a shell located here with this argument. */
#define SHELL_PATH "/bin/jsh"
#define SHELL_CMDLINE "/scr/start.jsh"

/* Initialization flag. */
static bool initialized = false;

typedef enum {
  RUN,
  STOP,
} state_t;
static state_t state = RUN;

/* Syslog buffer. */
static bd_t syslog_bd = -1;
static buffer_file_t syslog_buffer;

/* Queue contains requests for the vfs. */
static bd_t vfs_request_bda = -1;
static bd_t vfs_request_bdb = -1;
static vfs_request_queue_t vfs_request_queue;

/* Callbacks to execute from the vfs. */
static callback_queue_t vfs_response_queue;

static void
readfile_callback (void* data,
		   bd_t bda,
		   bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) != 0) {
    bfprintf (&syslog_buffer, ERROR "vfs provided bad readfile response\n");
    state = STOP;
    return;
  }

  if (error != VFS_SUCCESS) {
    bfprintf (&syslog_buffer, ERROR "could not read %s\n", SHELL_PATH);
    state = STOP;
    return;
  }

  bd_t bd1;
  bd_t bd2;

  argv_t argv;
  if (argv_initw (&argv, &bd1, &bd2) != 0) {
    bfprintf (&syslog_buffer, ERROR "could not initialize argv\n");
    state = STOP;
    return;
  }

  if (argv_append (&argv, SHELL_PATH, strlen (SHELL_PATH) + 1) != 0) {
    bfprintf (&syslog_buffer, ERROR "could not write to argv\n");
    state = STOP;
    return;
  }

  if (argv_append (&argv, SHELL_CMDLINE, strlen (SHELL_CMDLINE) + 1) != 0) {
    bfprintf (&syslog_buffer, ERROR "could not write to argv\n");
    state = STOP;
    return;
  }

  if (create (bdb, size, bd1, bd2, 0, 0, true) == -1) {
    bfprintf (&syslog_buffer, ERROR "could not create %s\n", SHELL_PATH);
    state = STOP;
    return;
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
    bfprintf (&syslog_buffer, ERROR "vfs provided bad mount response\n");
    state = STOP;
    return;
  }

  if (error != VFS_SUCCESS) {
    bfprintf (&syslog_buffer, ERROR "mounting tmpfs on %s failed\n", ROOT_PATH);
    state = STOP;
    return;
  }

  /* Request the shell. */
  vfs_request_queue_push_readfile (&vfs_request_queue, SHELL_PATH);
  callback_queue_push (&vfs_response_queue, readfile_callback, 0);
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    syslog_bd = buffer_create (0);
    if (syslog_bd == -1) {
      /* Nothing we can do. */
      exit ();
    }
    if (buffer_file_initw (&syslog_buffer, syslog_bd) != 0) {
      /* Nothing we can do. */
      exit ();
    }

    vfs_request_bda = buffer_create (0);
    vfs_request_bdb = buffer_create (0);
    if (vfs_request_bda == -1 ||
	vfs_request_bdb == -1) {
      bfprintf (&syslog_buffer, ERROR "could not create vfs request buffers\n");
      state = STOP;
      return;
    }

    vfs_request_queue_init (&vfs_request_queue);
    callback_queue_init (&vfs_response_queue);

    aid_t aid = getaid ();
    bd_t bda = getinita ();
    bd_t bdb = getinitb ();

    cpio_archive_t archive;
    if (cpio_archive_init (&archive, bda) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not initialize cpio archive\n");
      state = STOP;
      return;
    }
    
    cpio_file_t* syslog_file = 0;
    cpio_file_t* vfs_file = 0;
    cpio_file_t* tmpfs_file = 0;
    cpio_file_t* file;
    while ((file = cpio_archive_next_file (&archive)) != 0) {
      if (strcmp (file->name, SYSLOG_PATH) == 0) {
	syslog_file = file;
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
    
    /* Create the syslog. */
    if (syslog_file != 0) {
      aid_t syslog_aid = create (syslog_file->bd, syslog_file->size, -1, -1, SYSLOG_NAME, strlen (SYSLOG_NAME) + 1, false);
      if (syslog_aid != -1) {

	/* Bind to the syslog. */
	description_t syslog_description;
	if (description_init (&syslog_description, syslog_aid) != 0) {
	  exit ();
	}
	
	const ano_t syslog_stdin = description_name_to_number (&syslog_description, SYSLOG_STDIN, strlen (SYSLOG_STDIN) + 1);
	
	description_fini (&syslog_description);
	
	/* We bind the response first so they don't get lost. */
	if (bind (getaid (), SYSLOG_NO, 0, syslog_aid, syslog_stdin, 0) == -1) {
	  exit ();
	}
      }

      cpio_file_destroy (syslog_file);
    }

    /* Create the vfs. */
    if (vfs_file == 0) {
      bfprintf (&syslog_buffer, ERROR "no vfs file\n");
      state = STOP;
      return;
    }
    aid_t vfs = create (vfs_file->bd, vfs_file->size, -1, -1, VFS_NAME, strlen (VFS_NAME) + 1, false);
    cpio_file_destroy (vfs_file);
    if (vfs == -1) {
      bfprintf (&syslog_buffer, ERROR "could not create vfs\n");
      state = STOP;
      return;
    }
    
    /* Bind to the vfs so we can mount the tmpfs. */
    description_t vfs_description;
    if (description_init (&vfs_description, vfs) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not describe vfs\n");
      state = STOP;
      return;
    }
    
    /* If these actions don't exist, then attempts to bind below will fail. */
    const ano_t vfs_request = description_name_to_number (&vfs_description, VFS_REQUEST_NAME, strlen (VFS_REQUEST_NAME) + 1);
    const ano_t vfs_response = description_name_to_number (&vfs_description, VFS_RESPONSE_NAME, strlen (VFS_RESPONSE_NAME) + 1);
    
    description_fini (&vfs_description);
    
    /* We bind the response first so they don't get lost. */
    if (bind (vfs, vfs_response, 0, aid, VFS_RESPONSE_NO, 0) == -1 ||
	bind (aid, VFS_REQUEST_NO, 0, vfs, vfs_request, 0) == -1) {
      bfprintf (&syslog_buffer, ERROR "could not bind to vfs\n");
      state = STOP;
      return;
    }
    
    /* Create the tmpfs. */
    if (tmpfs_file == 0) {
      bfprintf (&syslog_buffer, ERROR "no tmpfs file\n");
      state = STOP;
      return;
    }
    /* Note:  We pass the buffer containing the cpio archive. */
    aid_t tmpfs = create (tmpfs_file->bd, tmpfs_file->size, bda, -1, 0, 0, false);
    cpio_file_destroy (tmpfs_file);
    if (tmpfs == -1) {
      bfprintf (&syslog_buffer, ERROR "could not create tmpfs\n");
      state = STOP;
      return;
    }
    
    /* Mount tmpfs on ROOT_PATH. */
    vfs_request_queue_push_mount (&vfs_request_queue, tmpfs, ROOT_PATH);
    callback_queue_push (&vfs_response_queue, mount_callback, 0);

    if (bda != -1) {
      buffer_destroy (bda);
    }
    if (bdb != -1) {
      buffer_destroy (bdb);
    }
  }
}

/* init
   ----
   Init receives a buffer containing a cpio archive.
   It looks for the files registry, vfs, tmpfs, and init and creates automata from them except for init.
   It passes the cpio archive to tmpfs.
   
   Post: mount_state == MOUNT && tmpfs != -1 && init_file != 0
 */
BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  end_internal_action ();
}

/* stop
   ----
   Stop the automaton.
   
   Pre:  state == STOP and syslog_buffer is empty
   Post: 
*/
static bool
stop_precondition (void)
{
  return state == STOP && buffer_file_size (&syslog_buffer) == 0;
}

BEGIN_INTERNAL (NO_PARAMETER, STOP_NO, "", "", stop, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (ano, param);

  if (stop_precondition ()) {
    exit ();
  }
  end_internal_action ();
}

/* syslog
   ------
   Output error messages.
   
   Pre:  syslog_buffer is not empty
   Post: syslog_buffer is empty
*/
static bool
syslog_precondition (void)
{
  return buffer_file_size (&syslog_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, SYSLOG_NO, "", "", syslogx, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (ano, param);

  if (syslog_precondition ()) {
    buffer_file_truncate (&syslog_buffer);
    end_output_action (true, syslog_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
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

BEGIN_OUTPUT (NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (VFS_REQUEST_NO, param);

  if (vfs_request_precondition ()) {
    if (vfs_request_queue_pop_to_buffer (&vfs_request_queue, vfs_request_bda, vfs_request_bdb) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not write vfs request\n");
      state = STOP;
      end_output_action (false, -1, -1);
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
BEGIN_INPUT (NO_PARAMETER, VFS_RESPONSE_NO, "", "", vfs_response, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (callback_queue_empty (&vfs_response_queue)) {
    bfprintf (&syslog_buffer, WARNING "vfs produced spurious response\n");
    end_input_action (bda, bdb);
  }

  const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
  callback_t callback = callback_queue_item_callback (item);
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&vfs_response_queue);

  callback (data, bda, bdb);

  end_input_action (bda, bdb);
}

void
schedule (void)
{
  if (stop_precondition ()) {
    scheduler_add (STOP_NO, 0);
  }
  if (syslog_precondition ()) {
    scheduler_add (SYSLOG_NO, 0);
  }
  if (vfs_request_precondition ()) {
    scheduler_add (VFS_REQUEST_NO, 0);
  }
}
