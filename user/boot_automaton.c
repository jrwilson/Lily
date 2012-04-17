#include <automaton.h>
#include <string.h>
#include <callback_queue.h>
#include <description.h>
#include <dymem.h>
#include "cpio.h"
#include "vfs_msg.h"
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

/* Paths in the cpio archive. */
#define SYSLOG_PATH "bin/syslog"
#define VFS_PATH "bin/vfs"
#define TMPFS_PATH "bin/tmpfs"

/* Mount the tmpfs here. */
#define ROOT_PATH "/"

/* Create a shell located here with this argument. */
#define SHELL_PATH "/bin/jsh"
#define SCRIPT_PATH "/scr/start.jsh"

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
  if (read_vfs_readfile_response (0, bda, &error, &size) != 0) {
    buffer_file_puts (&syslog_buffer, 0, ERROR "vfs provided bad readfile response\n");
    state = STOP;
    return;
  }

  if (error != VFS_SUCCESS) {
    bfprintf (&syslog_buffer, 0, ERROR "could not read %s\n", SHELL_PATH);
    state = STOP;
    return;
  }

  bd_t bd1 = buffer_create (0, 0);
  if (bd1 == -1) {
    buffer_file_puts (&syslog_buffer, 0, ERROR "could not create argument buffer\n");
    state = STOP;
    return;
  }

  buffer_file_t bf;
  if (buffer_file_initw (&bf, 0, bd1) != 0) {
    buffer_file_puts (&syslog_buffer, 0, ERROR "could not initialize argument buffer\n");
    state = STOP;
    return;
  }

  if (bfprintf (&bf, 0, "script=%s\n", SCRIPT_PATH) != 0) {
    buffer_file_puts (&syslog_buffer, 0, ERROR "could not append to argument buffer\n");
    state = STOP;
    return;
  }

  if (creates (0, bdb, size, bd1, -1, 0, true) == -1) {
    bfprintf (&syslog_buffer, 0, ERROR "could not create %s\n", SHELL_PATH);
    state = STOP;
    return;
  }

  buffer_destroy (0, bd1);

  /* The bdb buffer will be destroyed by the input action that calls this callback. */
}

static void
mount_callback (void* data,
		bd_t bda,
		bd_t bdb)
{
  vfs_error_t error;
  if (read_vfs_mount_response (0, bda, bdb, &error) != 0) {
    buffer_file_puts (&syslog_buffer, 0, ERROR "vfs provided bad mount response\n");
    state = STOP;
    return;
  }

  if (error != VFS_SUCCESS) {
    bfprintf (&syslog_buffer, 0, ERROR "mounting tmpfs on %s failed\n", ROOT_PATH);
    state = STOP;
    return;
  }

  /* Request the shell. */
  vfs_request_queue_push_readfile (&vfs_request_queue, 0, SHELL_PATH);
  callback_queue_push (&vfs_response_queue, 0, readfile_callback, 0);
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    syslog_bd = buffer_create (0, 0);
    if (syslog_bd == -1) {
      /* Nothing we can do. */
      exit ();
    }
    if (buffer_file_initw (&syslog_buffer, 0, syslog_bd) != 0) {
      /* Nothing we can do. */
      exit ();
    }

    vfs_request_bda = buffer_create (0, 0);
    vfs_request_bdb = buffer_create (0, 0);
    if (vfs_request_bda == -1 ||
	vfs_request_bdb == -1) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not create vfs request buffers\n");
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
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not initialize cpio archive\n");
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
      aid_t syslog_aid = creates (0, syslog_file->bd, syslog_file->size, -1, -1, SYSLOG_NAME, false);
      if (syslog_aid != -1) {

	/* Bind to the syslog. */
	description_t syslog_description;
	if (description_init (&syslog_description, 0, syslog_aid) != 0) {
	  exit ();
	}
	
	action_desc_t desc;
	if (description_read_name (&syslog_description, &desc, SYSLOG_TEXT_IN) != 0) {
	  exit ();
	}

	/* We bind the response first so they don't get lost. */
	if (bind (0, getaid (), SYSLOG_NO, 0, syslog_aid, desc.number, 0) == -1) {
	  exit ();
	}
	
	description_fini (&syslog_description, 0);
      }

      cpio_file_destroy (syslog_file);
    }

    /* Create the vfs. */
    if (vfs_file == 0) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "no vfs file\n");
      state = STOP;
      return;
    }
    aid_t vfs = creates (0, vfs_file->bd, vfs_file->size, -1, -1, VFS_NAME, false);
    cpio_file_destroy (vfs_file);
    if (vfs == -1) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not create vfs\n");
      state = STOP;
      return;
    }
    
    /* Bind to the vfs so we can mount the tmpfs. */
    description_t vfs_description;
    if (description_init (&vfs_description, 0, vfs) != 0) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not describe vfs\n");
      state = STOP;
      return;
    }
    
    action_desc_t vfs_request;
    if (description_read_name (&vfs_description, &vfs_request, VFS_REQUEST_NAME) != 0) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not describe vfs\n");
      state = STOP;
      return;
    }

    action_desc_t vfs_response;
    if (description_read_name (&vfs_description, &vfs_response, VFS_RESPONSE_NAME) != 0) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not describe vfs\n");
      state = STOP;
      return;
    }
    
    /* We bind the response first so they don't get lost. */
    if (bind (0, vfs, vfs_response.number, 0, aid, VFS_RESPONSE_NO, 0) == -1 ||
	bind (0, aid, VFS_REQUEST_NO, 0, vfs, vfs_request.number, 0) == -1) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not bind to vfs\n");
      state = STOP;
      return;
    }

    description_fini (&vfs_description, 0);
    
    /* Create the tmpfs. */
    if (tmpfs_file == 0) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "no tmpfs file\n");
      state = STOP;
      return;
    }
    /* Note:  We pass the buffer containing the cpio archive. */
    aid_t tmpfs = creates (0, tmpfs_file->bd, tmpfs_file->size, bda, -1, 0, false);
    cpio_file_destroy (tmpfs_file);
    if (tmpfs == -1) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not create tmpfs\n");
      state = STOP;
      return;
    }
    
    /* Mount tmpfs on ROOT_PATH. */
    vfs_request_queue_push_mount (&vfs_request_queue, 0, tmpfs, ROOT_PATH);
    callback_queue_push (&vfs_response_queue, 0, mount_callback, 0);

    if (bda != -1) {
      buffer_destroy (0, bda);
    }
    if (bdb != -1) {
      buffer_destroy (0, bdb);
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
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

  if (stop_precondition ()) {
    exit ();
  }
  finish_internal ();
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

BEGIN_OUTPUT (NO_PARAMETER, SYSLOG_NO, "", "", syslog, ano_t ano, int param)
{
  initialize ();

  if (syslog_precondition ()) {
    buffer_file_truncate (&syslog_buffer);
    finish_output (true, syslog_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

/* vfs_request
   -------------
   Send a request to the vfs.

   Pre:  state == RUN && vfs request queue is not empty
   Post: vfs request queue is empty
 */
static bool
vfs_request_precondition (void)
{
  return state == RUN && !vfs_request_queue_empty (&vfs_request_queue);
}

BEGIN_OUTPUT (NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, ano_t ano, int param)
{
  initialize ();

  if (vfs_request_precondition ()) {
    if (vfs_request_queue_pop_to_buffer (&vfs_request_queue, 0, vfs_request_bda, vfs_request_bdb) != 0) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not write vfs request\n");
      state = STOP;
      finish_output (false, -1, -1);
    }
    finish_output (true, vfs_request_bda, vfs_request_bdb);
  }
  else {
    finish_output (false, -1, -1);
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

  if (state == RUN) {
    if (callback_queue_empty (&vfs_response_queue)) {
      buffer_file_puts (&syslog_buffer, 0, WARNING "vfs produced spurious response\n");
      finish_input (bda, bdb);
    }
    
    const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
    callback_t callback = callback_queue_item_callback (item);
    void* data = callback_queue_item_data (item);
    callback_queue_pop (&vfs_response_queue);
    
    callback (data, bda, bdb);
  }

  finish_input (bda, bdb);
}

void
do_schedule (void)
{
  if (stop_precondition ()) {
    schedule (0, STOP_NO, 0);
  }
  if (syslog_precondition ()) {
    schedule (0, SYSLOG_NO, 0);
  }
  if (vfs_request_precondition ()) {
    schedule (0, VFS_REQUEST_NO, 0);
  }
}
