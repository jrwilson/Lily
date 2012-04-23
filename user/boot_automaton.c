#include <automaton.h>
#include <string.h>
#include "cpio.h"
#include "data_stack.h"
#include "environment.h"

/*
  The Boot Automaton
  ==================
  The goal of the boot automaton is to create a minimal environment that allows automata to load other automata from a store.
  The environment consists of three optional automata:
    1.  syslog - The syslog automaton receives log events and outputs them as text.
    2.  tmpfs - The tmpfs automaton implements a simple file system based on the contents of a cpio archive.  (The buffer supplied to the boot automaton is passed to the tmpfs automaton.)
    3.  finda - The finda automaton allows automata to find each other.
  After attempting to create these automata, the boot automaton loads a shell automaton to continue the boot process.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define INIT_NO 1

/* Paths in the cpio archive. */
#define SYSLOG_PATH "bin/syslog"
#define TMPFS_PATH "bin/tmpfs"
#define FINDA_PATH "bin/finda"
#define JSH_PATH "bin/jsh"

/* Initialization flag. */
static bool initialized = false;

#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "
#define INFO __FILE__ ": info: "

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    bd_t bda = getinita ();
    bd_t bdb = getinitb ();

    cpio_archive_t archive;
    if (cpio_archive_init (&archive, bda) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize cpio archive: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    aid_t finda_aid = -1;
    aid_t tmpfs_aid = -1;
    bd_t jsh_bd = -1;
    cpio_file_t file;
    while (cpio_archive_read (&archive, &file) == 0) {
      if ((file.mode & CPIO_TYPE_MASK) == CPIO_REGULAR) {
	if (strcmp (file.name, SYSLOG_PATH) == 0) {
	  bd_t syslog_bd = cpio_file_read (&archive, &file);
	  if (syslog_bd == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read syslog image: %s\n", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  aid_t syslog_aid = create (syslog_bd, -1, -1, false);
	  if (syslog_aid == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create syslog automaton: %s\n", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  if (buffer_destroy (syslog_bd) != 0) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy syslog image: %s\n", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "syslog = %d\n", syslog_aid);
	  logs (log_buffer);
	}
	else if (strcmp (file.name, TMPFS_PATH) == 0) {
	  bd_t tmpfs_bd = cpio_file_read (&archive, &file);
	  if (tmpfs_bd == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read tmpfs image: %s\n", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  tmpfs_aid = create (tmpfs_bd, -1, -1, false);
	  if (tmpfs_aid == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create tmpfs automaton: %s\n", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  if (buffer_destroy (tmpfs_bd) != 0) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy tmpfs image: %s\n", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "tmpfs = %d\n", tmpfs_aid);
	  logs (log_buffer);
	}
	else if (strcmp (file.name, FINDA_PATH) == 0) {
	  bd_t finda_bd = cpio_file_read (&archive, &file);
	  if (finda_bd == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read finda image: %s\n", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  finda_aid = create (finda_bd, -1, -1, false);
	  if (finda_aid == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create finda automaton: %s\n", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  if (buffer_destroy (finda_bd) != 0) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy finda image: %s\n", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "finda = %d\n", finda_aid);
	  logs (log_buffer);
	}
	else if (strcmp (file.name, JSH_PATH) == 0) {
	  jsh_bd = cpio_file_read (&archive, &file);
	}
      }
    }

    if (bda != -1) {
      if (buffer_destroy (bda) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy init buffer: %s\n", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
    }
    if (bdb != -1) {
      if (buffer_destroy (bdb) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy init buffer: %s\n", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
    }

    bd_t ds_bd = buffer_create (0);
    data_stack_t ds;
    if (ds_bd == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (data_stack_initw (&ds, ds_bd) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    if (data_stack_push_table (&ds) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // {}
    if (data_stack_push_string (&ds, FINDA) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // {} FINDA
    if (data_stack_push_integer (&ds, finda_aid) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // {} FINDA finda_aid
    if (data_stack_insert (&ds) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid }
    if (data_stack_push_string (&ds, FS) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid } FS
    if (data_stack_push_table (&ds) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid } FS {}
    if (data_stack_push_string (&ds, ROOT_PATH) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid } FS {} ROOT_PATH
    if (data_stack_push_integer (&ds, tmpfs_aid) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid } FS {} ROOT_PATH tmpfs_aid
    if (data_stack_insert (&ds) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid } FS { ROOT_PATH = tmpfs_aid }
    if (data_stack_insert (&ds) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid; FS = { ROOT_PATH = tmpfs_aid } }
    if (data_stack_push_string (&ds, ARGS) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid; FS = { ROOT_PATH = tmpfs_aid } } ARGS
    if (data_stack_push_table (&ds) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid; FS = { ROOT_PATH = tmpfs_aid } } ARGS {}
    if (data_stack_push_string (&ds, "script") != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid; FS = { ROOT_PATH = tmpfs_aid } } ARGS {} "script"
    if (data_stack_push_string (&ds, "/scr/start.jsh") != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid; FS = { ROOT_PATH = tmpfs_aid } } ARGS {} "script" "/scr/start.jsh"
    if (data_stack_insert (&ds) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid; FS = { ROOT_PATH = tmpfs_aid } } ARGS { "script" = "/scr/start.jsh" }
    if (data_stack_insert (&ds) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    // { FINDA = finda_aid; FS = { ROOT_PATH = tmpfs_aid }; ARGS = { "script" = "/scr/start.jsh" } }

    if (jsh_bd == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read jsh image: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    aid_t jsh_aid = create (jsh_bd, ds_bd, -1, false);
    if (jsh_aid == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create jsh automaton: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (buffer_destroy (jsh_bd) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy jsh image: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    if (buffer_destroy (ds_bd) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy ds buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "jsh = %d\n", jsh_aid);
    logs (log_buffer);
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

void
do_schedule (void)
{ }

