#include <automaton.h>
#include <io.h>
#include <string.h>
#include <buffer_queue.h>
#include <callback_queue.h>
#include <fifo_scheduler.h>
#include <description.h>
#include "vfs_msg.h"
#include "argv.h"

#define DESTROY_BUFFERS_NO 1
#define VFS_RESPONSE_NO 2
#define VFS_REQUEST_NO 3

#define VFS_NAME "vfs"

/* Initialization flag. */
static bool initialized = false;

/* Queue of buffers that need to be destroyed. */
static buffer_queue_t destroy_queue;

/* The aid of the vfs. */
static aid_t vfs_aid = -1;

/* Messages to the vfs. */
static buffer_queue_t vfs_request_queue;

/* Callbacks to execute from the vfs. */
static callback_queue_t vfs_response_queue;

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
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb);

static void
readfile_callback (void* data,
		   bd_t bda,
		   bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) == -1) {
    ssyslog ("jsh: error: vfs provide bad readfile response\n");
    exit ();
  }

  if (error != VFS_SUCCESS) {
    ssyslog ("jsh: error: readfile failed \n");
    exit ();
  }

  /* TODO */
  const char* str = buffer_map (bdb);
  syslog (str, size);
}

/* init
   ----
   The script to execute comes in on bda.

   Post: ???
 */
BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t aid, bd_t bda, bd_t bdb)
{
  ssyslog ("jsh: init\n");
  initialize ();

  /* Bind to the vfs. */
  vfs_aid = lookup (VFS_NAME, strlen (VFS_NAME));
  if (vfs_aid == -1) {
    ssyslog ("jsh: error: no vfs\n");
    exit ();
  }

  description_t vfs_description;
  if (description_init (&vfs_description, vfs_aid) == -1) {
    ssyslog ("jsh: error: couldn't describe vfs\n");
    exit ();
  }
  const ano_t vfs_request = description_name_to_number (&vfs_description, VFS_REQUEST_NAME);
  const ano_t vfs_response = description_name_to_number (&vfs_description, VFS_RESPONSE_NAME);
  description_fini (&vfs_description);

  if (vfs_request == NO_ACTION ||
      vfs_response == NO_ACTION) {
    ssyslog ("jsh: error: the vfs does not appear to be a vfs\n");
    exit ();
  }

  /* We bind the response first so they don't get lost. */
  if (bind (vfs_aid, vfs_response, 0, aid, VFS_RESPONSE_NO, 0) == -1 ||
      bind (aid, VFS_REQUEST_NO, 0, vfs_aid, vfs_request, 0) == -1) {
    ssyslog ("jsh: error: Couldn't bind to vfs\n");
    exit ();
  }

  argv_t argv;
  size_t argc;
  if (argv_initr (&argv, bda, bdb, &argc) != -1) {
    if (argc >= 2) {
      /* Request the script. */
      const char* filename = argv_arg (&argv, 1);

      bd_t bd = write_vfs_readfile_request (filename);
      if (bd == -1) {
	ssyslog ("jsh: error: Couldn't create readfile request\n");
	exit ();
      }
      buffer_queue_push (&vfs_request_queue, 0, bd, -1);
      callback_queue_push (&vfs_response_queue, readfile_callback, 0);
    }
  }

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
    ssyslog ("jsh: vfs_request\n");
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
  ssyslog ("jsh: vfs_response\n");
  initialize ();

  if (callback_queue_empty (&vfs_response_queue)) {
    ssyslog ("jsh: error: vfs produced spurious response\n");
    exit ();
  }

  const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
  callback_t callback = callback_queue_item_callback (item);
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&vfs_response_queue);

  callback (data, bda, bdb);

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

  scheduler_finish (output_fired, bda, bdb);
}
