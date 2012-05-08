#include <automaton.h>
#include <description.h>
#include <dymem.h>
#include <string.h>
#include "cpio.h"
#include "de.h"
#include "environment.h"

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
#define INIT_OUT_NO 2

/* Paths in the cpio archive. */
#define TMPFS_PATH "bin/tmpfs"
#define JSH_PATH "bin/jsh"

#define INIT_NAME "init"

/* Logging. */
#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "
#define INFO __FILE__ ": info: "
#define TODO __FILE__ ": todo: "

/* Initialization flag. */
static bool initialized = false;

/* Initialization buffers. */
static bd_t init_output_bda = -1;
static bd_t init_output_bdb = -1;

typedef struct init_item init_item_t;
struct init_item {
  aid_t aid;
  bd_t bda;
  bd_t bdb;
  init_item_t* next;
};

static init_item_t* init_head = 0;
static init_item_t** init_tail = &init_head;

static void
push_init (aid_t aid,
	   bd_t bda,
	   bd_t bdb)
{
  init_item_t* i = malloc (sizeof (init_item_t));
  memset (i, 0, sizeof (init_item_t));
  i->aid = aid;
  i->bda = buffer_copy (bda);
  i->bdb = buffer_copy (bdb);

  *init_tail = i;
  init_tail = &i->next;
}

static void
pop_init (void)
{
  init_item_t* i = init_head;
  init_head = i->next;
  if (init_head == 0) {
    init_tail = &init_head;
  }

  buffer_destroy (i->bda);
  buffer_destroy (i->bdb);
  free (i);
}

BEGIN_SYSTEM (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  if (!initialized) {
    initialized = true;

    aid_t this_aid = getaid ();

    init_output_bda = buffer_create (0);
    init_output_bdb = buffer_create (0);
    if (init_output_bda == -1 || init_output_bdb == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create output buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    bd_t bda = get_boot_data ();
    if (bda != -1) {
      cpio_archive_t archive;
      if (cpio_archive_init (&archive, bda) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize cpio archive: %s", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
      
      bd_t tmpfs_bd = -1;
      bd_t jsh_bd = -1;

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

      aid_t tmpfs_aid = create (tmpfs_bd, false);
      if (tmpfs_aid == -1) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create tmpfs: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }

      /* Bind to the init action of the tmpfs. */

      description_t description;
      action_desc_t desc;

      description_init (&description, tmpfs_aid);
      if (description_read_name (&description, &desc, INIT_NAME, 0) == 0 && desc.type == LILY_ACTION_INPUT) {
        if (bind (this_aid, INIT_OUT_NO, tmpfs_aid, tmpfs_aid, desc.number, 0) == -1) {
	  snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not bind to tmpfs init action: %s", lily_error_string (lily_error));
	  logs (log_buffer);
	  exit (-1);
	}
      }
      else {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "tmpfs does not have an init action");
	logs (log_buffer);
	exit (-1);
      }
      description_fini (&description);
      
      /* Execute the init action of the tmpfs. */
      push_init (tmpfs_aid, bda, -1);

      if (tmpfs_bd == -1) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "no tmpfs image");
	logs (log_buffer);
	exit (-1);
      }

      aid_t jsh_aid = create (jsh_bd, true);
      if (jsh_aid == -1) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create jsh: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }

      /* Bind to the init action of the jsh. */
      description_init (&description, jsh_aid);
      if (description_read_name (&description, &desc, INIT_NAME, 0) == 0 && desc.type == LILY_ACTION_INPUT) {
        if (bind (this_aid, INIT_OUT_NO, jsh_aid, jsh_aid, desc.number, 0) == -1) {
	  snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not bind to tmpfs init action: %s", lily_error_string (lily_error));
	  logs (log_buffer);
	  exit (-1);
	}
      }
      else {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "jsh does not have an init action");
	logs (log_buffer);
	exit (-1);
      }
      description_fini (&description);

      /* Execute the init action of the jsh. */
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
      
      push_init (jsh_aid, de_bd, -1);
      
      if (buffer_destroy (de_bd) != 0) {
        snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy de buffer: %s", lily_error_string (lily_error));
        logs (log_buffer);
        exit (-1);
      }

      if (jsh_bd == -1) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "no jsh image");
	logs (log_buffer);
	exit (-1);
      }

      if (buffer_destroy (tmpfs_bd) != 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy tmpfs image: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }

      if (buffer_destroy (jsh_bd) != 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy tmpfs image: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }
      
      if (buffer_destroy (bda) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy init buffer: %s", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
    }
    else {
      snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "no boot data");
      logs (log_buffer);
    }
  }

  finish_internal ();
}

BEGIN_OUTPUT (AUTO_PARAMETER, INIT_OUT_NO, "", "", init_out, ano_t ano, aid_t aid)
{
  if (init_head != 0 && init_head->aid == aid) {
    bd_t bda = -1;
    size_t bda_size = buffer_size (init_head->bda);
    if (bda_size != -1) {
      bda = init_output_bda;
      buffer_assign (bda, init_head->bda, 0, bda_size);
    }

    bd_t bdb = -1;
    size_t bdb_size = buffer_size (init_head->bdb);
    if (bdb_size != -1) {
      bdb = init_output_bdb;
      buffer_assign (bdb, init_head->bdb, 0, bdb_size);
    }

    pop_init ();
    finish_output (true, bda, bdb);
  }
  finish_output (false, -1, -1);
}

void
do_schedule (void)
{
  if (init_head != 0) {
    schedule (INIT_OUT_NO, init_head->aid);
  }
}
