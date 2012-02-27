#include <automaton.h>
#include <io.h>
#include <string.h>
#include <buffer_queue.h>
#include <callback_queue.h>
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
#define LAUNCH_SHELL_NO 6

#define SHELL_PATH "/bin/jsh"
#define SCRIPT_PATH "/scr/start.jsh"

/* Our aid. */
static aid_t init_aid = -1;

/* Initialization flag. */
static bool initialized = false;

/* Queue of buffers that need to be destroyed. */
static buffer_queue_t destroy_queue;

/* Message for querying the registry for the vfs. */
static bd_t query_bd = -1;

/* Messages to the vfs. */
static buffer_queue_t vfs_request_queue;

/* Callbacks to execute from the vfs. */
static callback_queue_t vfs_response_queue;

/* Buffer containing the text of the shell. */
static bd_t shell_bd = -1;

/* Buffer containing the text of the script. */
static bd_t script_bd = -1;

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
    buffer_queue_init (&vfs_request_queue);
    callback_queue_init (&vfs_response_queue);
  }
}

static void
shell_callback (void* data,
		 bd_t bda,
		 bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  
  if (read_vfs_readfile_response (bda, &error, &size) == -1 || error != VFS_SUCCESS) {
    ssyslog ("init: error: couldn't read " SHELL_PATH "\n");
    exit ();
  }

  shell_bd = buffer_copy (bdb, 0, buffer_size (bdb));
}

static void
script_callback (void* data,
		 bd_t bda,
		 bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  
  if (read_vfs_readfile_response (bda, &error, &size) == -1 || error != VFS_SUCCESS) {
    ssyslog ("init: error: couldn't read " SCRIPT_PATH "\n");
    exit ();
  }
  
  script_bd = buffer_copy (bdb, 0, buffer_size (bdb));
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

  {
    bd_t bd = write_vfs_readfile_request (SHELL_PATH);
    if (bd == -1) {
      ssyslog ("init: error: Couldn't create readfile request\n");
      exit ();
    }
    buffer_queue_push (&vfs_request_queue, 0, bd, -1);
    callback_queue_push (&vfs_response_queue, shell_callback, 0);
  }

  {
    bd_t bd = write_vfs_readfile_request (SCRIPT_PATH);
    if (bd == -1) {
      ssyslog ("init: error: Couldn't create readfile request\n");
      exit ();
    }
    buffer_queue_push (&vfs_request_queue, 0, bd, -1);
    callback_queue_push (&vfs_response_queue, script_callback, 0);
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
    ssyslog ("init: vfs_request\n");
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
  ssyslog ("init: vfs_response\n");
  initialize ();

  if (callback_queue_empty (&vfs_response_queue)) {
    ssyslog ("init: error: vfs produced spurious response\n");
    exit ();
  }

  const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
  callback_t callback = callback_queue_item_callback (item);
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&vfs_response_queue);

  callback (data, bda, bdb);

  end_action (false, bda, bdb);
}

/* launch_shell
   ------------
   Launch the shell will the specified script.

   Pre:  shell_bd != -1 && script_bd != -1
   Post: shell_bd == -1 && script_bd == -1
 */
static bool
launch_shell_precondition (void)
{
  return shell_bd != -1 && script_bd != -1;
}

BEGIN_INTERNAL (NO_PARAMETER, LAUNCH_SHELL_NO, "", "", launch_shell, int param)
{
  initialize ();
  scheduler_remove (LAUNCH_SHELL_NO, param);

  if (launch_shell_precondition ()) {
    ssyslog ("init: launch_shell\n");

    if (create (shell_bd, true, script_bd) == -1) {
      ssyslog ("init: error: couldn't create shell automaton\n");
      exit ();
    }

    buffer_destroy (shell_bd);
    shell_bd = -1;
    buffer_destroy (script_bd);
    script_bd = -1;
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

  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }
  if (query_request_precondition ()) {
    scheduler_add (QUERY_REQUEST_NO, 0);
  }
  if (vfs_request_precondition ()) {
    scheduler_add (VFS_REQUEST_NO, 0);
  }
  if (launch_shell_precondition ()) {
    scheduler_add (LAUNCH_SHELL_NO, 0);
  }

  scheduler_finish (output_fired, bda, bdb);
}
