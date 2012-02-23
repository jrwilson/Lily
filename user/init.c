#include <automaton.h>
#include <io.h>
#include <string.h>
#include <buffer_queue.h>
#include <fifo_scheduler.h>
#include <description.h>
#include "registry_msg.h"
#include "vfs_msg.h"

/*
  Init
  ====
  The boot automaton creates the init automaton after it has created the registry, the vfs, and mounted a temporary file system on the vfs.
  Consequently, the init automaton can use the contents of the file system to continue the boot process.
  Currently, the init automaton reads a file and displays its content.
  
  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define DESTROY_BUFFERS_NO 1
#define QUERY_RESPONSE_NO 2
#define QUERY_REQUEST_NO 3

/* Initialization flag. */
static bool initialized = false;

/* Queue of buffers that need to be destroyed. */
static buffer_queue_t destroy_queue;

/* Message for querying the registry for the vfs. */
static bd_t query_bd = -1;
static size_t query_bd_size = 0;

/* The aid of the vfs. */
static aid_t vfs_aid = -1;

static void
ssyslog (const char* msg)
{
  syslog (msg, strlen (msg));
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    buffer_queue_init (&destroy_queue);
  }
}

static void
end_action (bool output_fired,
	    bd_t bd,
	    size_t bd_size);

/* init
   ----
   Formulate a message that queries the registry for the vfs.

   Post: query_bd != -1
 */
BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t aid, bd_t bd, size_t bd_size)
{
  ssyslog ("init: init\n");
  initialize ();

  /* Look up the registry. */
  aid_t registry_aid = get_registry ();
  if (registry_aid == -1) {
    ssyslog ("init: error: no registry\n");
    exit ();
  }

  description_t desc;
  if (description_init (&desc, registry_aid) == -1) {
    ssyslog ("init: error: no registry description\n");
    exit ();
  }

  const ano_t request = description_name_to_number (&desc, REGISTRY_QUERY_REQUEST_NAME);
  const ano_t response = description_name_to_number (&desc, REGISTRY_QUERY_RESPONSE_NAME);

  description_fini (&desc);

  /* Bind to the response first so they don't get lost. */
  if (bind (registry_aid, response, 0, aid, QUERY_RESPONSE_NO, 0) == -1 ||
      bind (aid, QUERY_REQUEST_NO, 0, registry_aid, request, 0) == -1) {
    ssyslog ("init: error: couldn't bind to registry\n");
    exit ();
  }

  query_bd_size = 0;
  query_bd = write_registry_query_request (REGISTRY_STRING_EQUAL, VFS_DESCRIPTION, VFS_DESCRIPTION_SIZE, &query_bd_size);

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

/* query_request
   -------------
   Query the registry for the vfs.

   Pre:  query_bd != -1
   Post: query_bd == -1
 */
static bool
query_request_precondition (void)
{
  return query_bd != -1;
}

BEGIN_OUTPUT (NO_PARAMETER, QUERY_REQUEST_NO, "", "", query_request, int param, size_t bc)
{
  ssyslog ("init: query_request\n");
  initialize ();
  scheduler_remove (QUERY_REQUEST_NO, param);

  if (query_request_precondition ()) {
    bd_t bd = query_bd;
    size_t bd_size = query_bd_size;

    query_bd = -1;
    query_bd_size = 0;

    end_action (true, bd, bd_size);
  }
  else {
    end_action (false, -1, 0);
  }
}

/* query_response
   --------------
   Extract the aid of the vfs from the response from the registry.

   Post: vfs_aid != -1
 */
BEGIN_INPUT (NO_PARAMETER, QUERY_RESPONSE_NO, "", "", query_response, int param, bd_t bd, size_t bd_size)
{
  ssyslog ("init: query_response\n");
  initialize ();

  registry_query_response_t r;
  registry_error_t error;
  registry_method_t method;
  size_t count;

  if (registry_query_response_initr (&r, bd, bd_size, &error, &method, &count) == -1) {
    ssyslog ("init: error: couldn't read registry response\n");
    exit ();
  }
  
  if (error != REGISTRY_SUCCESS ||
      method != REGISTRY_STRING_EQUAL) {
    ssyslog ("init: error: bad registry response\n");
    exit ();
  }

  const void* description;
  size_t size;

  if (count != 1 || registry_query_response_read (&r, &vfs_aid, &description, &size) == -1) {
    ssyslog ("init: error: no vfs\n");
    exit ();
  }

  /* TODO:  Bind to vfs. */

  end_action (false, bd, bd_size);
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

  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }
  if (query_request_precondition ()) {
    scheduler_add (QUERY_REQUEST_NO, 0);
  }

  scheduler_finish (output_fired, bd);
}

/* #include "keyboard.h" */
/* #include "kb_us_104.h" */
/* #include "shell.h" */
/* #include "terminal.h" */
/* #include "vga.h" */

  /* aid_t keyboard = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "keyboard") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     keyboard = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t kb_us_104 = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "kb_us_104") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     kb_us_104 = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t shell = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "shell") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     shell = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t terminal = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "terminal") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     terminal = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t vga = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "vga") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     vga = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* bind (keyboard, KEYBOARD_SCAN_CODE, 0, kb_us_104, KB_US_104_SCAN_CODE, 0); */
  /* bind (kb_us_104, KB_US_104_STRING, 0, shell, SHELL_STDIN, 0); */
  /* bind (shell, SHELL_STDOUT, 0, terminal, TERMINAL_DISPLAY, 0); */
  /* bind (terminal, TERMINAL_VGA_OP, 0, vga, VGA_OP, 0); */
