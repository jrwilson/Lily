#include <automaton.h>
#include <string.h>
#include "cpio.h"

/*
  The Boot Automaton
  ==================
  The goal of the boot automaton is to create an environment that allows automata to load other automata from a store.
  The boot automaton receives a buffer containing a cpio archive containing programs for a vfs, tmpfs, and shell from which is tries to create automata.
  The buffer supplied to the boot automaton is passed to the tmpfs automaton.
  The boot automaton also attempts to create a logging automaton.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define INIT_NO 1
#define VFS_REQUEST_NO 2
#define VFS_RESPONSE_NO 3

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

/* /\* Queue contains requests for the vfs. *\/ */
/* static bd_t vfs_request_bda = -1; */
/* static bd_t vfs_request_bdb = -1; */
/* static vfs_request_queue_t vfs_request_queue; */

/* /\* Callbacks to execute from the vfs. *\/ */
/* static callback_queue_t vfs_response_queue; */

#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "

/* static void */
/* readfile_callback (void* data, */
/* 		   bd_t bda, */
/* 		   bd_t bdb) */
/* { */
/*   vfs_error_t error; */
/*   size_t size; */
/*   if (read_vfs_readfile_response (bda, &error, &size) != 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read readfile response: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   if (error != VFS_SUCCESS) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "vfs error: %s", vfs_error_string (error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   bd_t bd1 = buffer_create (0); */
/*   if (bd1 == -1) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create shell argument buffer: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   buffer_file_t bf; */
/*   if (buffer_file_initw (&bf, bd1) != 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize shell argument buffer: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   if (bfprintf (&bf, "script=%s\n", SCRIPT_PATH) < 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to shell argument buffer: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   if (creates (bdb, size, bd1, -1, 0, true) == -1) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create shell automaton: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   if (buffer_destroy (bd1) != 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy shell argument buffer: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   /\* The bdb buffer will be destroyed by the input action that calls this callback. *\/ */
/* } */

/* static void */
/* mount_callback (void* data, */
/* 		bd_t bda, */
/* 		bd_t bdb) */
/* { */
/*   vfs_error_t error; */
/*   if (read_vfs_mount_response (bda, bdb, &error) != 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read readfile response: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   if (error != VFS_SUCCESS) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "vfs error: %s", vfs_error_string (error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   /\* Request the shell. *\/ */
/*   if (vfs_request_queue_push_readfile (&vfs_request_queue, SHELL_PATH) != 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not push vfs request: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */
/*   if (callback_queue_push (&vfs_response_queue, readfile_callback, 0) != 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not push vfs callback: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */
/* } */

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    bd_t bda = getinita ();
    bd_t bdb = getinitb ();

    cpio_archive_t archive;
    if (cpio_archive_init (&archive, bda) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize cpio archive: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
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
    	if (cpio_file_destroy (file) != 0) {
    	  snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy cpio archive: %s", lily_error_string (lily_error));
    	  logs (log_buffer);
    	  exit (-1);
    	}
      }
    }

    if (lily_error != LILY_ERROR_SUCCESS) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "error parsing cpio archive: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    /* Create the syslog. */
    if (syslog_file != 0) {
      aid_t syslog_aid = create (syslog_file->bd, syslog_file->size, -1, -1, false);
      if (syslog_aid == -1) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create syslog automaton: %s", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
      if (cpio_file_destroy (syslog_file) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy cpio archive: %s", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
    }

    /* Create the vfs. */
    if (vfs_file == 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "no vfs file");
      logs (log_buffer);
      exit (-1);
    }
    aid_t vfs = create (vfs_file->bd, vfs_file->size, -1, -1, false);
    if (vfs == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create vfs automaton: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (cpio_file_destroy (vfs_file) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy cpio archive: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    /* Create the tmpfs. */
    if (tmpfs_file == 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "no tmpfs file");
      logs (log_buffer);
      exit (-1);
    }
    /* Note:  We pass the buffer containing the cpio archive. */
    aid_t tmpfs = create (tmpfs_file->bd, tmpfs_file->size, bda, -1, false);
    if (tmpfs == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create tmpfs automaton: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (cpio_file_destroy (tmpfs_file) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy cpio archive: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    if (bda != -1) {
      if (buffer_destroy (bda) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy init buffer: %s", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
    }
    if (bdb != -1) {
      if (buffer_destroy (bdb) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy init buffer: %s", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
    }

    /* vfs_request_bda = buffer_create (0); */
    /* vfs_request_bdb = buffer_create (0); */
    /* if (vfs_request_bda == -1 || */
    /* 	vfs_request_bdb == -1) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create vfs request buffer: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */

    /* vfs_request_queue_init (&vfs_request_queue); */
    /* callback_queue_init (&vfs_response_queue); */

    /* aid_t aid = getaid (); */
    
    /* /\* Bind to the vfs so we can mount the tmpfs. *\/ */
    /* description_t vfs_description; */
    /* if (description_init (&vfs_description, vfs) != 0) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create vfs description: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
    
    /* action_desc_t vfs_request; */
    /* if (description_read_name (&vfs_description, &vfs_request, VFS_REQUEST_NAME) != 0) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "vfs automaton does not contain request action"); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */

    /* action_desc_t vfs_response; */
    /* if (description_read_name (&vfs_description, &vfs_response, VFS_RESPONSE_NAME) != 0) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "vfs automaton does not contain response action"); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
    
    /* /\* We bind the response first so they don't get lost. *\/ */
    /* if (bind (vfs, vfs_response.number, 0, aid, VFS_RESPONSE_NO, 0) == -1) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not bind to vfs response: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
    /* if (bind (aid, VFS_REQUEST_NO, 0, vfs, vfs_request.number, 0) == -1) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not bind to vfs request: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */

    /* if (description_fini (&vfs_description) != 0) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy vfs description: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
        
    /* /\* Mount tmpfs on ROOT_PATH. *\/ */
    /* if (vfs_request_queue_push_mount (&vfs_request_queue, tmpfs, ROOT_PATH) != 0) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not push vfs request: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
    /* if (callback_queue_push (&vfs_response_queue, mount_callback, 0) != 0) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not push vfs callback: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

/* /\* vfs_request */
/*    ------------- */
/*    Send a request to the vfs. */

/*    Pre:  state == RUN && vfs request queue is not empty */
/*    Post: vfs request queue is empty */
/*  *\/ */
/* static bool */
/* vfs_request_precondition (void) */
/* { */
/*   return !vfs_request_queue_empty (&vfs_request_queue); */
/* } */

/* BEGIN_OUTPUT (NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, ano_t ano, int param, size_t bc) */
/* { */
/*   initialize (); */

/*   if (vfs_request_precondition ()) { */
/*     if (vfs_request_queue_pop_to_buffer (&vfs_request_queue, vfs_request_bda, vfs_request_bdb) != 0) { */
/*       snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not pop vfs request: %s", lily_error_string (lily_error)); */
/*       logs (log_buffer); */
/*       exit (-1); */
/*     } */
/*     finish_output (true, vfs_request_bda, vfs_request_bdb); */
/*   } */
/*   else { */
/*     finish_output (false, -1, -1); */
/*   } */
/* } */

/* /\* vfs_response */
/*    -------------- */
/*    Extract the aid of the vfs from the response from the registry. */

/*    Post: ??? */
/*  *\/ */
/* BEGIN_INPUT (NO_PARAMETER, VFS_RESPONSE_NO, "", "", vfs_response, ano_t ano, int param, bd_t bda, bd_t bdb) */
/* { */
/*   initialize (); */

/*   if (callback_queue_empty (&vfs_response_queue)) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "spurious vfs response"); */
/*     logs (log_buffer); */
/*     finish_input (bda, bdb); */
/*   } */

/*   const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue); */
/*   callback_t callback = callback_queue_item_callback (item); */
/*   void* data = callback_queue_item_data (item); */
/*   callback_queue_pop (&vfs_response_queue); */
  
/*   callback (data, bda, bdb); */

/*   finish_input (bda, bdb); */
/* } */

void
do_schedule (void)
{
  /* if (vfs_request_precondition ()) { */
  /*   if (schedule (VFS_REQUEST_NO, 0) != 0) { */
  /*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not schedule vfs request action: %s", lily_error_string (lily_error)); */
  /*     logs (log_buffer); */
  /*     exit (-1); */
  /*   } */
  /* } */
}
