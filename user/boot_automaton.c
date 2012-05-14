#include <automaton.h>
#include <description.h>
#include <dymem.h>
#include <string.h>
#include "cpio.h"
#include "de.h"
#include "environment.h"
#include "system.h"

/*
  The Boot Automaton
  ==================
  The boot automaton is the first automaton loaded and consequently has the responsibility of creating an environment for loading additional automata.
  This environment consists of a temporary file system (tmpfs) and another automaton (typically a shell called jsh) from which the rest of the system can be loaded.
  The boot automaton receives a buffer containing a cpio archive that it uses to create these two automata.
  It passes the archive to the tmpfs automaton.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define INIT_NO 1
#define SYSTEM_ACTION_NO 2
#define INIT_OUT_NO 3
#define INIT_IN_NO 4
#define STATUS_REQUEST_OUT_NO 5
#define STATUS_REQUEST_IN_NO 6
#define STATUS_RESPONSE_OUT_NO 7
#define STATUS_RESPONSE_IN_NO 8
#define BINDING_UPDATE_OUT_NO 9
#define BINDING_UPDATE_IN_NO 10


/* Paths in the cpio archive. */
#define TMPFS_PATH "bin/tmpfs"
#define JSH_PATH "bin/jsh"

/* Logging. */
#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "
#define INFO __FILE__ ": info: "
#define TODO __FILE__ ": todo: "

/* Initialization flag. */
static bool initialized = false;

static bool loaded = false;

static system_t system;

static bd_t jsh_bd = -1;

static void
jsh_callback (void* arg,
	      automaton_t* a)
{
  aid_t jsh_aid = automaton_aid (a);
  if (jsh_aid == -1) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create jsh: %s", lily_error_string (automaton_error (a)));
    logs (log_buffer);
    exit (-1);
  }
}

static void
tmpfs_callback (void* arg,
		automaton_t* a)
{
  aid_t tmpfs_aid = automaton_aid (a);
  if (tmpfs_aid == -1) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create tmpfs: %s", lily_error_string (automaton_error (a)));
    logs (log_buffer);
    exit (-1);
  }

  /* Form the init buffer of the jsh. */
  bd_t de_bd = buffer_create (0);
  if (de_bd == -1) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create de buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }
  buffer_file_t de_buffer;
  if (buffer_file_initw (&de_buffer, de_bd) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize de buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }
  
  de_val_t* root = de_create_object ();
  de_set (root, "." FS "[0].aid", de_create_integer (tmpfs_aid));
  de_set (root, "." FS "[0].name", de_create_string ("bootfs"));
  de_set (root, "." FS "[0].nodeid", de_create_integer (0));
  de_set (root, "." ACTIVE_FS, de_create_integer (tmpfs_aid));
  de_set (root, "." ARGS "." "script", de_create_string ("/scr/start.jsh"));
  de_serialize (root, &de_buffer);
  de_destroy (root);
  
  automaton_t* jsh = system_create (&system, jsh_bd, true, de_bd, -1, jsh_callback, 0);
  
  if (buffer_destroy (de_bd) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy de buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }

  if (buffer_destroy (jsh_bd) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy tmpfs image: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }
  jsh_bd = -1;
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    if (system_init (&system) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize system");
      logs (log_buffer);
      exit (-1);
    }
  }
}

static void
load (bd_t bda)
{
  if (!loaded) {
    loaded = true;

    if (bda != -1) {
      cpio_archive_t archive;
      if (cpio_archive_init (&archive, bda) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize cpio archive: %s", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
      
      bd_t tmpfs_bd = -1;

      cpio_file_t file;
      while (cpio_archive_read (&archive, &file) == 0) {
    	if ((file.mode & CPIO_TYPE_MASK) == CPIO_REGULAR) {
    	  if (strcmp (file.name, TMPFS_PATH) == 0) {
    	    tmpfs_bd = cpio_file_read (&archive, &file);
    	    if (tmpfs_bd == -1) {
    	      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read tmpfs image: %s", lily_error_string (lily_error));
    	      logs (log_buffer);
    	      exit (-1);
    	    }
    	  }
    	  else if (strcmp (file.name, JSH_PATH) == 0) {
    	    jsh_bd = cpio_file_read (&archive, &file);
    	    if (jsh_bd == -1) {
    	      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read jsh image: %s", lily_error_string (lily_error));
    	      logs (log_buffer);
    	      exit (-1);
    	    }
    	  }
    	}
      }

      if (tmpfs_bd == -1) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "no tmpfs image");
    	logs (log_buffer);
    	exit (-1);
      }
      
      if (jsh_bd == -1) {
      	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "no jsh image");
      	logs (log_buffer);
      	exit (-1);
      }

      /* Create the tmpfs. */
      automaton_t* tmpfs = system_create (&system, tmpfs_bd, false, bda, -1, tmpfs_callback, 0);

      if (buffer_destroy (tmpfs_bd) != 0) {
      	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy tmpfs image: %s", lily_error_string (lily_error));
      	logs (log_buffer);
      	exit (-1);
      }
    }
    else {
      snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "no initialization data");
      logs (log_buffer);
    }
  }

}

BEGIN_SYSTEM (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  bd_t bda = get_boot_data ();
  load (bda);
  buffer_destroy (bda);
  finish_internal ();
}

BEGIN_SYSTEM (NO_PARAMETER, SYSTEM_ACTION_NO, SA_SYSTEM_ACTION_NAME, "", system_action, ano_t ano, int param)
{
  initialize ();
  system_system_action (&system);
}

BEGIN_OUTPUT (AUTO_PARAMETER, INIT_OUT_NO, SA_INIT_OUT_NAME, "", init_out, ano_t ano, aid_t aid)
{
  initialize ();
  system_init_out (&system, aid);
}

BEGIN_INPUT (NO_PARAMETER, INIT_IN_NO, SA_INIT_IN_NAME, "", init_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();
  load (bda);
  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, STATUS_REQUEST_OUT_NO, SA_STATUS_REQUEST_OUT_NAME, "", status_request_out, ano_t ano, aid_t aid)
{
  initialize ();
  system_status_request_out (&system, aid);
}

BEGIN_INPUT (AUTO_PARAMETER, STATUS_REQUEST_IN_NO, SA_STATUS_REQUEST_IN_NAME, "", status_request_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  system_status_request_in (&system, aid, bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, STATUS_RESPONSE_OUT_NO, SA_STATUS_RESPONSE_OUT_NAME, "", status_response_out, ano_t ano, aid_t aid)
{
  initialize ();
  system_status_response_out (&system, aid);
}

BEGIN_INPUT (AUTO_PARAMETER, STATUS_RESPONSE_IN_NO, SA_STATUS_RESPONSE_IN_NAME, "", status_response_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  system_status_response_in (&system, aid, bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, BINDING_UPDATE_OUT_NO, SA_BINDING_UPDATE_OUT_NAME, "", binding_update_out, ano_t ano, aid_t aid)
{
  initialize ();
  system_binding_update_out (&system, aid);
}

BEGIN_INPUT (AUTO_PARAMETER, BINDING_UPDATE_IN_NO, SA_BINDING_UPDATE_IN_NAME, "", binding_update_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  system_binding_update_in (&system, aid, bda, bdb);
}

void
do_schedule (void)
{
  system_schedule (&system);
}
