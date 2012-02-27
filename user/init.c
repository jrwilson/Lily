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
#define VFS_RESPONSE_NO 4
#define VFS_REQUEST_NO 5

/* Our aid. */
static aid_t init_aid = -1;

/* Initialization flag. */
static bool initialized = false;

/* Queue of buffers that need to be destroyed. */
static buffer_queue_t destroy_queue;

/* Message for querying the registry for the vfs. */
static bd_t query_bd = -1;

/* Message to the vfs. */
static bd_t vfs_bd = -1;

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
	    bd_t bda,
	    bd_t bdb);

/* init
   ----
   Formulate a message that queries the registry for the vfs.

   Post: query_bd != -1
 */
BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t aid, bd_t bda, bd_t bdb)
{
  init_aid = aid;

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

  query_bd = write_registry_query_request (REGISTRY_STRING_EQUAL, VFS_DESCRIPTION, VFS_DESCRIPTION_SIZE);

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

/* query_request
   -------------
   Query the registry for the vfs.
   We check the init_aid so we can bind when we get the response.

   Pre:  query_bd != -1 && init_aid != -1
   Post: query_bd == -1
 */
static bool
query_request_precondition (void)
{
  return query_bd != -1 && init_aid != -1;
}

BEGIN_OUTPUT (NO_PARAMETER, QUERY_REQUEST_NO, "", "", query_request, int param)
{
  initialize ();
  scheduler_remove (QUERY_REQUEST_NO, param);

  if (query_request_precondition ()) {
    ssyslog ("init: query_request\n");
    bd_t bd = query_bd;

    query_bd = -1;

    end_action (true, bd, -1);
  }
  else {
    end_action (false, -1, -1);
  }
}

/* query_response
   --------------
   Extract the aid of the vfs from the response from the registry.
   Request a file from the vfs.

   Post: vfs_bd != -1
 */
BEGIN_INPUT (NO_PARAMETER, QUERY_RESPONSE_NO, "", "", query_response, int param, bd_t bda, bd_t bdb)
{
  ssyslog ("init: query_response\n");
  initialize ();

  registry_query_response_t r;
  registry_error_t error;
  registry_method_t method;
  size_t count;

  if (registry_query_response_initr (&r, bda, &error, &method, &count) == -1) {
    ssyslog ("init: error: couldn't read registry response\n");
    exit ();
  }
  
  if (error != REGISTRY_SUCCESS ||
      method != REGISTRY_STRING_EQUAL) {
    ssyslog ("init: error: bad registry response\n");
    exit ();
  }

  aid_t vfs_aid;
  const void* description;
  size_t size;

  if (count != 1 || registry_query_response_read (&r, &vfs_aid, &description, &size) == -1) {
    ssyslog ("init: error: no vfs\n");
    exit ();
  }

  description_t vfs_description;
  if (description_init (&vfs_description, vfs_aid) == -1) {
    ssyslog ("init: error: Could not describe vfs\n");
    exit ();
  }

  /* If these actions don't exist, then attempts to bind below will fail. */
  const ano_t vfs_request = description_name_to_number (&vfs_description, VFS_REQUEST_NAME);
  const ano_t vfs_response = description_name_to_number (&vfs_description, VFS_RESPONSE_NAME);

  description_fini (&vfs_description);

  /* We bind the response first so they don't get lost. */
  if (bind (vfs_aid, vfs_response, 0, init_aid, VFS_RESPONSE_NO, 0) == -1 ||
      bind (init_aid, VFS_REQUEST_NO, 0, vfs_aid, vfs_request, 0) == -1) {
    ssyslog ("init: error: Couldn't bind to vfs\n");
    exit ();
  }

  vfs_bd = write_vfs_readfile_request ("/hello.txt");
  if (vfs_bd == -1) {
    ssyslog ("init: error: Couldn't create readfile request\n");
    exit ();
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
  return vfs_bd != -1;
}

BEGIN_OUTPUT (NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, int param)
{
  initialize ();
  scheduler_remove (VFS_REQUEST_NO, param);

  if (vfs_request_precondition ()) {
    ssyslog ("init: vfs_request\n");
    bd_t bd = vfs_bd;
    vfs_bd = -1;

    end_action (true, bd, -1);
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
  ssyslog ("init: vfs_response\n");
  initialize ();

  vfs_error_t error;
  size_t size;

  if (read_vfs_readfile_response (bda, &error, &size) == -1) {
    ssyslog ("init: error: couldn't read hello.txt\n");
    exit ();
  }

  const char* str = buffer_map (bdb);
  syslog (str, size);

  /* registry_vfs_response_t r; */
  /* registry_error_t error; */
  /* registry_method_t method; */
  /* size_t count; */

  /* if (registry_vfs_response_initr (&r, bd, bd_size, &error, &method, &count) == -1) { */
  /* } */
  
  /* if (error != REGISTRY_SUCCESS || */
  /*     method != REGISTRY_STRING_EQUAL) { */
  /*   ssyslog ("init: error: bad registry response\n"); */
  /*   exit (); */
  /* } */

  /* aid_t vfs_aid; */
  /* const void* description; */
  /* size_t size; */

  /* if (count != 1 || registry_vfs_response_read (&r, &vfs_aid, &description, &size) == -1) { */
  /*   ssyslog ("init: error: no vfs\n"); */
  /*   exit (); */
  /* } */

  /* description_t vfs_description; */
  /* if (description_init (&vfs_description, vfs_aid) == -1) { */
  /*   ssyslog ("init: error: Could not describe vfs\n"); */
  /*   exit (); */
  /* } */

  /* /\* If these actions don't exist, then attempts to bind below will fail. *\/ */
  /* const ano_t vfs_request = description_name_to_number (&vfs_description, VFS_REQUEST_NAME); */
  /* const ano_t vfs_response = description_name_to_number (&vfs_description, VFS_RESPONSE_NAME); */

  /* description_fini (&vfs_description); */

  /* /\* We bind the response first so they don't get lost. *\/ */
  /* if (bind (vfs_aid, vfs_response, 0, init_aid, VFS_RESPONSE_NO, 0) == -1 || */
  /*     bind (init_aid, VFS_REQUEST_NO, 0, vfs_aid, vfs_request, 0) == -1) { */
  /*   ssyslog ("init: error: Couldn't bind to vfs\n"); */
  /*   exit (); */
  /* } */

  /* /\* TODO:  Prepare request to vfs. *\/ */

  end_action (false, bda, bdb);
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

  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }
  if (query_request_precondition ()) {
    scheduler_add (QUERY_REQUEST_NO, 0);
  }
  if (vfs_request_precondition ()) {
    scheduler_add (VFS_REQUEST_NO, 0);
  }

  scheduler_finish (output_fired, bda, bdb);
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
