#include "vfs_msg.h"
#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <buffer_file.h>
#include <buffer_queue.h>
#include <description.h>
#include "registry_msg.h"

/*
  VFS
  ===
  The virtual file system presents a unified hierarchical namespace for managing files.
  The VFS translates high-level requests from clients into low-level file-system operations.
  
  Client 1              File system A
  Client 2  <-> VFS <-> File system B
  Client 3              File system C
  ...                   ...
  
  Implementation
  --------------
  The current implementation is a simple queue processor.
  Client requests are placed on a queue and processed in FIFO order.
  Client responses are placed on a queue and processed in FIFO order per client.
  The core piece is the logic that translates client requests into file system requests.

  This implementation is not very efficient as it serializes everything.
  Performance can be increased by exploiting situations where client requests are independent and the fact that file system are independent.
  Thus, instead of having one pair of queues for client requests, one could have a queue for each client.
  Similar optimizations apply for the file systems.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define VFS_REGISTER_REQUEST_NO 1
#define VFS_REGISTER_RESPONSE_NO 2
#define VFS_REQUEST_NO 3
#define VFS_RESPONSE_NO 4
#define VFS_FS_REQUEST_NO 5
#define VFS_FS_RESPONSE_NO 6
#define DESTROY_BUFFERS_NO 7
#define DECODE_NO 8

#define DESCRIPTION "vfs"

/* A virtual inode. */
typedef struct {
  aid_t aid;	/* The automaton holding this file system. */
  size_t inode;	/* The inode in that file system. */
} vnode_t;

typedef struct mount_table_item mount_table_item_t;
struct mount_table_item {
  vnode_t a;
  vnode_t b;
  mount_table_item_t* next;
};

/* Initialization flag. */
static bool initialized = false;

/* Register message. */
static bd_t register_bd = -1;
static size_t register_bd_size = 0;

/* Queue for client requests. */
static buffer_queue_t client_request_queue;

/* Queue for client responses. */
static buffer_queue_t client_response_queue;

/* Variables for a message going to a file system. */
static aid_t fs_aid = -1;
static bd_t fs_bd = -1;
static size_t fs_bd_size = 0;

/* Queue of buffers to destroy. */
static buffer_queue_t destroy_queue;

/* Table to translate vnodes based on mounting. */
static mount_table_item_t* mount_table_head = 0;

/* State machine for client requests. */
static aid_t client_request_aid = -1;
static bd_t client_request_bd = -1;
static size_t client_request_bd_size = 0;
static vfs_type_t client_request_type;
static aid_t client_request_aid_arg;
static const char* client_request_path;
static size_t client_request_path_size;

static vnode_t path_lookup_current;
static bool path_lookup_done;

static void
end_action (bool output_fired,
	    bd_t bd,
	    size_t bd_size);

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    buffer_queue_init (&client_request_queue);
    buffer_queue_init (&client_response_queue);
    buffer_queue_init (&destroy_queue);
  }
}

static void
ssyslog (const char* msg)
{
  syslog (msg, strlen (msg));
}

static bool
vnode_equal (const vnode_t* x,
	     const vnode_t* y)
{
  return x->aid == y->aid && x->inode == y->inode;
}

/* init
   ----
   Binds to the registry and produces a registration message.
   
   Post: register_bd != -1
 */
BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t vfs_aid, bd_t bd, size_t bd_size)
{
  ssyslog ("vfs: init\n");
  initialize ();

  /* Look up the registry. */
  aid_t registry_aid = get_registry ();
  if (registry_aid == -1) {
    ssyslog ("vfs: warning: no registry\n");
    end_action (false, bd, bd_size);
  }

  description_t desc;
  if (description_init (&desc, registry_aid) == -1) {
    ssyslog ("vfs: warning: no registry description\n");
    end_action (false, bd, bd_size);
  }

  const ano_t register_request = description_name_to_number (&desc, REGISTRY_REGISTER_REQUEST_NAME);
  const ano_t register_response = description_name_to_number (&desc, REGISTRY_REGISTER_RESPONSE_NAME);

  description_fini (&desc);

  /* Bind to the response first so they don't get lost. */
  if (bind (registry_aid, register_response, 0, vfs_aid, VFS_REGISTER_RESPONSE_NO, 0) == -1 ||
      bind (vfs_aid, VFS_REGISTER_REQUEST_NO, 0, registry_aid, register_request, 0) == -1) {
    ssyslog ("vfs: warning: couldn't bind to registry\n");
    end_action (false, bd, bd_size);
  }

  register_bd_size = 0;
  register_bd = write_registry_register_request (REGISTRY_STRING_EQUAL, DESCRIPTION, strlen (DESCRIPTION) + 1, &register_bd_size);

  end_action (false, bd, bd_size);
}

/* register_request
   ----------------
   Send the registration message to the registry.

   Pre:  register_bd != -1
   Post: register_bd == -1
 */
static bool
register_request_precondition (void)
{
  return register_bd != -1;
}

BEGIN_OUTPUT (NO_PARAMETER, VFS_REGISTER_REQUEST_NO, "", "", reqister_request, int param, size_t bc)
{
  ssyslog ("vfs: register_request\n");

  initialize ();
  scheduler_remove (VFS_REGISTER_REQUEST_NO, param);

  if (register_request_precondition ()) {
    bd_t bd = register_bd;
    size_t bd_size = register_bd_size;

    register_bd = -1;
    register_bd_size = 0;

    end_action (true, bd, bd_size);
  }
  else {
    end_action (false, -1, 0);
  }
}

/* register_response
   -----------------
   Receive a response to the registration message.

   Post: none
 */
BEGIN_INPUT (NO_PARAMETER, VFS_REGISTER_RESPONSE_NO, "", "", register_response, int param, bd_t bd, size_t bd_size)
{
  ssyslog ("vfs: register_response\n");
  initialize ();

  registry_error_t error = REGISTRY_SUCCESS;
  if (read_registry_register_response (bd, bd_size, &error) == 0) {
    switch (error) {
    case REGISTRY_SUCCESS:
      /* Okay. */
      break;
    default:
      {
	ssyslog ("vfs: warning: failed to register\n");
      }
      break;
    }
  }
  else {
    ssyslog ("vfs: warning: couldn't map registry response\n");
  }

  end_action (false, bd, bd_size);
}

/* client_request
   --------------
   Receive a request from a client.

   Post: the request is at the end of the client_request_queue
 */
BEGIN_INPUT (AUTO_PARAMETER, VFS_REQUEST_NO, VFS_REQUEST_NAME, "", client_request, aid_t aid, bd_t bd, size_t bd_size)
{
  ssyslog ("vfs: client_request\n");
  initialize ();
  buffer_queue_push (&client_request_queue, aid, bd, bd_size);
  end_action (false, -1, 0);
}

/* client_response
   ---------------
   Send a response to a client.

   Pre:  The client indicated by aid has a response in the queue
   Post: The first response for the client indicated by aid is removed from the queue
 */
BEGIN_OUTPUT (AUTO_PARAMETER, VFS_RESPONSE_NO, VFS_RESPONSE_NAME, "", client_response, aid_t aid, size_t bc)
{
  ssyslog ("vfs: client_response\n");
  initialize ();
  scheduler_remove (VFS_RESPONSE_NO, aid);

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&client_response_queue, aid);

  if (item != 0) {
    /* Found a response.  Execute. */
    bd_t bd = buffer_queue_item_bd (item);
    size_t bd_size = buffer_queue_item_size (item);
    buffer_queue_erase (&client_response_queue, item);

    end_action (true, bd, bd_size);
  }
  else {
    /* Did not find a response. */
    end_action (false, -1, 0);
  }
}

/* file_system_request
   -------------------
   Send a request to a file system.

   Pre:  fs_aid != -1 && fs_bd != -1 && fs_bd_size != 0
   Post: fs_aid == -1 && fs_bd == -1 && fs_bd_size == 0
 */
static bool
file_system_request_precondition (aid_t aid)
{
  return fs_aid == aid && fs_aid != -1 && fs_bd != -1 && fs_bd_size != 0;
}

BEGIN_OUTPUT (AUTO_PARAMETER, VFS_FS_REQUEST_NO, "", "", file_system_request, aid_t aid, size_t bc)
{
  ssyslog ("vfs: file_system_request\n");
  initialize ();
  scheduler_remove (VFS_FS_REQUEST_NO, aid);

  if (file_system_request_precondition (aid)) {
    bd_t bd = fs_bd;
    size_t bd_size = fs_bd_size;

    fs_aid = -1;
    fs_bd = -1;
    fs_bd_size = 0;

    end_action (true, bd, bd_size);
  }
  else {
    /* Did not find a request. */
    end_action (false, -1, 0);
  }
}

/* file_system_response
   --------------------
   Process a response from a file system.

   Post: 
 */
BEGIN_INPUT (AUTO_PARAMETER, VFS_FS_RESPONSE_NO, "", "", file_system_response, aid_t aid, bd_t bd, size_t bd_size)
{
  ssyslog ("vfs: file_system_response\n");
  initialize ();
  /* TODO */
  end_action (false, bd, bd_size);
}

/* destroy_buffers
   ---------------
   Destroy buffers that have accumulated.  Useful for destroying a buffer after an output has produced it.

   Pre:  destroy_queue is not empty
   Post: destroy_queue is empty
 */
static bool
destroy_buffers_precondition (void)
{
  return !buffer_queue_empty (&destroy_queue);
}

BEGIN_INTERNAL (NO_PARAMETER, DESTROY_BUFFERS_NO, "", "", destroy_buffers, int param)
{
  ssyslog ("vfs: destroy_buffers\n");
  initialize ();
  scheduler_remove (DESTROY_BUFFERS_NO, param);
  if (destroy_buffers_precondition ()) {
    /* Drain the queue. */
    while (!buffer_queue_empty (&destroy_queue)) {
      buffer_destroy (buffer_queue_item_bd (buffer_queue_front (&destroy_queue)));
      buffer_queue_pop (&destroy_queue);
    }
  }

  end_action (false, -1, 0);
}

static void
form_response (vfs_type_t type,
	       vfs_error_t error)
{
  /* Create a response. */
  size_t bd_size;
  bd_t bd = write_vfs_response (type, error, &bd_size);
  buffer_queue_push (&client_response_queue, client_request_aid, bd, bd_size);
}

static void
reset_client_state (void)
{
  client_request_aid = -1;
  buffer_destroy (client_request_bd);
  client_request_bd = -1;
  client_request_bd_size = 0;
}

static bool
check_absolute_path (const char* path,
		     size_t path_size)
{
  /* path_size includes the null-terminator. */

  if (path_size < 2) {
    /* The smallest absolute path is "/".  This path is not big enough. */
    return false;
  }

  if (path[0] != '/') {
    /* Absolute path does not begin with /. */
    return false;
  }

  if (path[path_size - 1] != 0) {
    /* The path is not null-terminated. */
    return false;
  }

  if (strlen (path) + 1 != path_size) {
    /* Contains a null character. */
    return false;
  }
  
  return true;
}

static void
path_lookup_init (void)
{
  /* Start at the virtual root. */
  path_lookup_current.aid = -1;
  path_lookup_current.inode = 0;
  path_lookup_done = false;
}

static void
path_lookup_resume (void)
{
  if (!path_lookup_done) {
    /* First, look for an entry in the mount table. */
    mount_table_item_t* item;
    for (item = mount_table_head; item != 0; item = item->next) {
      if (vnode_equal (&item->a, &path_lookup_current)) {
	/* Found an entry. Translate and recur. */
	path_lookup_current = item->b;
	path_lookup_resume ();
	break;
      }
    }



    ssyslog ("vfs:  ####\n");
    syslog (client_request_path, client_request_path_size);
  }
  
  /* TODO */
}

/* decode
   ------
   Load the internal state machine with a client request.

   Pre:  the internal state machine is not loaded (client_request_bd == -1) and there is a request on the queue
   Post: the internal state machine is loaded (client_request_bd != -1) and the request is removed from the queue
 */
static bool
decode_precondition (void)
{
  return !buffer_queue_empty (&client_request_queue) && client_request_bd == -1;
}

BEGIN_INTERNAL (NO_PARAMETER, DECODE_NO, "", "", decode, int param)
{
  ssyslog ("vfs: decode\n");
  initialize ();
  scheduler_remove (DECODE_NO, param);
  if (decode_precondition ()) {
    /* Get the request from the queue. */
    const buffer_queue_item_t* front = buffer_queue_front (&client_request_queue);
    client_request_aid = buffer_queue_item_parameter (front);
    client_request_bd = buffer_queue_item_bd (front);
    client_request_bd_size = buffer_queue_item_size (front);
    buffer_queue_pop (&client_request_queue);

    if (read_vfs_request_type (client_request_bd, client_request_bd_size, &client_request_type) == -1) {
      form_response (VFS_UNKNOWN, VFS_BAD_REQUEST);
      reset_client_state ();
      end_action (false, -1, 0);
    }

    switch (client_request_type) {
    case VFS_MOUNT:
      {
	if (read_vfs_mount_request (client_request_bd, client_request_bd_size, &client_request_aid_arg, &client_request_path, &client_request_path_size) == -1) {
	  form_response (VFS_MOUNT, VFS_BAD_REQUEST);
	  reset_client_state ();
	  end_action (false, -1, 0);
	}
	
	if (!check_absolute_path (client_request_path, client_request_path_size)) {
	  form_response (VFS_MOUNT, VFS_BAD_PATH);
	  reset_client_state ();
	  end_action (false, -1, 0);
	}

	/*
	  To mount, we must:
	  1.  Traverse the existing virtual file system to find the vnode A indicated by the path.
	  2.  Bind to the file system.  Let B be the vnode representing the root of its file system.
	  3.  Insert an entry in the mount table that causes future path traversals to substitute A -> B.
	 */

	path_lookup_init ();
	path_lookup_resume ();
      }
      break;
    default:
      form_response (client_request_type, VFS_BAD_REQUEST_TYPE);
      reset_client_state ();
      end_action (false, -1, 0);
    }
  }

  end_action (false, -1, 0);
}

static void
end_action (bool output_fired,
	    bd_t bd,
	    size_t bd_size)
{
  if (bd != -1) {
    buffer_queue_push (&destroy_queue, 0, bd, bd_size);
  }

  if (register_request_precondition ()) {
    scheduler_add (VFS_REGISTER_REQUEST_NO, 0);
  }
  if (!buffer_queue_empty (&client_response_queue)) {
    scheduler_add (VFS_RESPONSE_NO, buffer_queue_item_parameter (buffer_queue_front (&client_response_queue)));
  }
  if (file_system_request_precondition (fs_aid)) {
    scheduler_add (VFS_FS_REQUEST_NO, fs_aid);
  }
  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }
  if (decode_precondition ()) {
    scheduler_add (DECODE_NO, 0);
  }

  scheduler_finish (output_fired, bd);
}
