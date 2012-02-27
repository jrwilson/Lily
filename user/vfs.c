#include "vfs_msg.h"
#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <buffer_file.h>
#include <buffer_queue.h>
#include <description.h>
#include <dymem.h>
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

  TODO:  Subscribe to the file systems when mounting to purge if they fail.

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
#define MOUNT_NO 9
#define READFILE_REQUEST_NO 10

/* A virtual node associates an node with a file system. */
typedef struct {
  vfs_fs_node_t node;
  aid_t aid;	/* The automaton (file system) that contains this node. */
} vnode_t;

typedef struct mount_item mount_item_t;
struct mount_item {
  vnode_t a;
  vnode_t b;
  mount_item_t* next;
};

/* The virtual root. */
static vnode_t root;

/* Our aid. */
static aid_t vfs_aid = -1;

/* Initialization flag. */
static bool initialized = false;

/* Register message. */
static bd_t register_bd = -1;

/* Queue for client requests. */
static buffer_queue_t client_request_queue;

/* Queue for client responses. */
static buffer_queue_t client_response_queue;

/* Variables for a message going to a file system. */
static aid_t fs_aid = -1;
static bd_t fs_bd = -1;
static vfs_fs_type_t fs_type;

/* Queue of buffers to destroy. */
static buffer_queue_t destroy_queue;

/* Table to translate vnodes based on mounting. */
static mount_item_t* mount_head = 0;

/* State machine for client requests. */
static aid_t client_request_aid = -1;
static bd_t client_request_bd = -1;
static vfs_type_t client_request_type;

/* State machine for looking up paths. */
static const char* path_lookup_path;
static size_t path_lookup_path_size;
static vnode_t path_lookup_current;
static bool path_lookup_done;
static bool path_lookup_error;
static const char* path_lookup_begin;
static const char* path_lookup_end;

/* State for mounts. */
static aid_t mount_aid;

/* State for readfile. */
static bool readfile_sent = false;

static void
end_action (bool output_fired,
	    bd_t bd);

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    root.node.type = DIRECTORY;
    root.node.id = 0;
    root.aid = -1;

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
  return x->aid == y->aid && x->node.id == y->node.id;
}

static mount_item_t*
mount_item_create (const vnode_t* a,
		   const vnode_t* b)
{
  mount_item_t* item = malloc (sizeof (mount_item_t));
  memset (item, 0, sizeof (mount_item_t));
  item->a = *a;
  item->b = *b;
  return item;
}

static void
form_unknown_response (vfs_error_t error)
{
  /* Create a response. */
  bd_t bd = write_vfs_unknown_response (error);
  buffer_queue_push (&client_response_queue, client_request_aid, bd);
}

static void
form_mount_response (vfs_error_t error)
{
  /* Create a response. */
  bd_t bd = write_vfs_mount_response (error);
  buffer_queue_push (&client_response_queue, client_request_aid, bd);
}

static void
form_readfile_response (vfs_error_t error)
{
  /* Create a response. */
  bd_t bd = write_vfs_readfile_response (error);
  buffer_queue_push (&client_response_queue, client_request_aid, bd);
}

static void
reset_client_state (void)
{
  client_request_aid = -1;
  buffer_destroy (client_request_bd);
  client_request_bd = -1;
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
  path_lookup_current = root;
  path_lookup_done = false;
  path_lookup_error = false;
  path_lookup_end = path_lookup_path;
}

static void
path_lookup_resume (void)
{
  if (!path_lookup_done) {
    /* First, look for an entry in the mount table. */
    mount_item_t* item;
    for (item = mount_head; item != 0; item = item->next) {
      if (vnode_equal (&item->a, &path_lookup_current)) {
	/* Found an entry. Translate and recur. */
	path_lookup_current = item->b;
	path_lookup_resume ();
	break;
      }
    }

    /* Advance to the next element in the path. */
    path_lookup_begin = path_lookup_end + 1;

    if (*path_lookup_begin == 0) {
      /* We are at the end of the path. */
      path_lookup_done = true;
      return;
    }
    
    /* Find the terminator (0) or next path separator (/). */
    path_lookup_end = path_lookup_begin + 1;
    while (*path_lookup_end != 0 && *path_lookup_end != '/') {
      ++path_lookup_end;
    }

    /* Ensure that the path search can continue. */
    if (path_lookup_current.aid == -1 || path_lookup_current.node.type != DIRECTORY) {
      /* No file system or not a directory. */
      path_lookup_done = true;
      path_lookup_error = true;
      return;
    }

    /* Form a message to a file system. */
    fs_aid = path_lookup_current.aid;
    fs_bd = write_vfs_fs_descend_request (path_lookup_current.node.id, path_lookup_begin, path_lookup_end - path_lookup_begin);
    fs_type = VFS_FS_DESCEND;
  }
}

/* init
   ----
   Binds to the registry and produces a registration message.
   
   Post: register_bd != -1
 */
BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t aid, bd_t bd)
{
  vfs_aid = aid;

  initialize ();

  /* Look up the registry. */
  aid_t registry_aid = get_registry ();
  if (registry_aid == -1) {
    ssyslog ("vfs: warning: no registry\n");
    end_action (false, bd);
  }

  description_t desc;
  if (description_init (&desc, registry_aid) == -1) {
    ssyslog ("vfs: warning: no registry description\n");
    end_action (false, bd);
  }

  const ano_t register_request = description_name_to_number (&desc, REGISTRY_REGISTER_REQUEST_NAME);
  const ano_t register_response = description_name_to_number (&desc, REGISTRY_REGISTER_RESPONSE_NAME);

  description_fini (&desc);

  /* Bind to the response first so they don't get lost. */
  if (bind (registry_aid, register_response, 0, vfs_aid, VFS_REGISTER_RESPONSE_NO, 0) == -1 ||
      bind (vfs_aid, VFS_REGISTER_REQUEST_NO, 0, registry_aid, register_request, 0) == -1) {
    ssyslog ("vfs: warning: couldn't bind to registry\n");
    end_action (false, bd);
  }

  register_bd = write_registry_register_request (REGISTRY_STRING_EQUAL, VFS_DESCRIPTION, VFS_DESCRIPTION_SIZE);

  end_action (false, bd);
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

    register_bd = -1;

    end_action (true, bd);
  }
  else {
    end_action (false, -1);
  }
}

/* register_response
   -----------------
   Receive a response to the registration message.

   Post: none
 */
BEGIN_INPUT (NO_PARAMETER, VFS_REGISTER_RESPONSE_NO, "", "", register_response, int param, bd_t bd)
{
  ssyslog ("vfs: register_response\n");
  initialize ();

  registry_error_t error = REGISTRY_SUCCESS;
  if (read_registry_register_response (bd, &error) == 0) {
    switch (error) {
    case REGISTRY_SUCCESS:
      /* Okay. */
      break;
    default:
      ssyslog ("vfs: warning: failed to register\n");
      break;
    }
  }
  else {
    ssyslog ("vfs: warning: couldn't map registry response\n");
  }

  end_action (false, bd);
}

/* client_request
   --------------
   Receive a request from a client.

   Post: the request is at the end of the client_request_queue
 */
BEGIN_INPUT (AUTO_PARAMETER, VFS_REQUEST_NO, VFS_REQUEST_NAME, "", client_request, aid_t aid, bd_t bd)
{
  ssyslog ("vfs: client_request\n");
  initialize ();
  buffer_queue_push (&client_request_queue, aid, bd);
  end_action (false, -1);
}

/* client_response
   ---------------
   Send a response to a client.

   Pre:  The client indicated by aid has a response in the queue
   Post: The first response for the client indicated by aid is removed from the queue
 */
BEGIN_OUTPUT (AUTO_PARAMETER, VFS_RESPONSE_NO, VFS_RESPONSE_NAME, "", client_response, aid_t aid, size_t bc)
{
  initialize ();
  scheduler_remove (VFS_RESPONSE_NO, aid);

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&client_response_queue, aid);

  if (item != 0) {
    ssyslog ("vfs: client_response\n");

    /* Found a response.  Execute. */
    bd_t bd = buffer_queue_item_bd (item);
    buffer_queue_erase (&client_response_queue, item);

    end_action (true, bd);
  }
  else {
    /* Did not find a response. */
    end_action (false, -1);
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
  return fs_aid == aid && fs_aid != -1 && fs_bd != -1;
}

BEGIN_OUTPUT (AUTO_PARAMETER, VFS_FS_REQUEST_NO, "", "", file_system_request, aid_t aid, size_t bc)
{
  initialize ();
  scheduler_remove (VFS_FS_REQUEST_NO, aid);

  if (file_system_request_precondition (aid)) {
    ssyslog ("vfs: file_system_request\n");
    bd_t bd = fs_bd;

    fs_aid = -1;
    fs_bd = -1;

    end_action (true, bd);
  }
  else {
    /* Did not find a request. */
    end_action (false, -1);
  }
}

/* file_system_response
   --------------------
   Process a response from a file system.

   Post: 
 */
BEGIN_INPUT (AUTO_PARAMETER, VFS_FS_RESPONSE_NO, "", "", file_system_response, aid_t aid, bd_t bd)
{
  ssyslog ("vfs: file_system_response\n");
  initialize ();

  switch (fs_type) {
  case VFS_FS_UNKNOWN:
    /* Do nothing. */
    break;
  case VFS_FS_DESCEND:
    {
      vfs_fs_error_t error;
      vfs_fs_node_t node;
      if (read_vfs_fs_descend_response (bd, &error, &node) == -1) {
	/* TODO:  This is a protocol violation. */
	exit ();
      }

      if (error != VFS_FS_SUCCESS) {
	/* A less serious error. */
	path_lookup_done = true;
	path_lookup_error = true;
	end_action (false, bd);
      }	

      /* Update the current location. */
      path_lookup_current.node = node;

      path_lookup_resume ();
    }
    break;
  case VFS_FS_READFILE:
    {
      vfs_fs_error_t error;
      if (read_vfs_fs_readfile_response (bd, &error) == -1) {
	/* TODO:  This is a protocol violation. */
	exit ();
      }

      if (error != VFS_FS_SUCCESS) {
	/* TODO:  We just looked up a file but could not read it.  Something is wrong. */
	exit ();
      }	

      /* TODO */
      ssyslog ("vfs: READFILE\n");
      form_readfile_response (VFS_SUCCESS);
    }
    break;
  }
  end_action (false, bd);
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
  initialize ();
  scheduler_remove (DESTROY_BUFFERS_NO, param);
  if (destroy_buffers_precondition ()) {
    /* Drain the queue. */
    while (!buffer_queue_empty (&destroy_queue)) {
      buffer_destroy (buffer_queue_item_bd (buffer_queue_front (&destroy_queue)));
      buffer_queue_pop (&destroy_queue);
    }
  }

  end_action (false, -1);
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
    buffer_queue_pop (&client_request_queue);

    if (read_vfs_request_type (client_request_bd, &client_request_type) == -1) {
      form_unknown_response (VFS_BAD_REQUEST);
      reset_client_state ();
      end_action (false, -1);
    }

    switch (client_request_type) {
    case VFS_MOUNT:
      {
	if (read_vfs_mount_request (client_request_bd, &mount_aid, &path_lookup_path, &path_lookup_path_size) == -1) {
	  form_mount_response (VFS_BAD_REQUEST);
	  reset_client_state ();
	  end_action (false, -1);
	}

	if (!check_absolute_path (path_lookup_path, path_lookup_path_size)) {
	  form_mount_response (VFS_BAD_PATH);
	  reset_client_state ();
	  end_action (false, -1);
	}

	/* See if the file system is already mounted.
	   If it is, its an error.
	*/
	mount_item_t* item;
	for (item = mount_head; item != 0; item = item->next) {
	  if (item->b.aid == mount_aid) {
	    form_mount_response (VFS_ALREADY_MOUNTED);
	    reset_client_state ();
	    end_action (false, -1);
	    break;
	  }
	}

	/* Start the process of converting the path to a vnode. */
	path_lookup_init ();
	path_lookup_resume ();
      }
      break;
    case VFS_READFILE:
      {
	if (read_vfs_readfile_request (client_request_bd, &path_lookup_path, &path_lookup_path_size) == -1) {
	  form_readfile_response (VFS_BAD_REQUEST);
	  reset_client_state ();
	  end_action (false, -1);
	}

	if (!check_absolute_path (path_lookup_path, path_lookup_path_size)) {
	  form_readfile_response (VFS_BAD_PATH);
	  reset_client_state ();
	  end_action (false, -1);
	}

	/* We have not send the readfile request yet. */
	readfile_sent = false;

	/* Start the process of converting the path to a vnode. */
	path_lookup_init ();
	path_lookup_resume ();
      }
      break;
    default:
      form_unknown_response (VFS_BAD_REQUEST_TYPE);
      reset_client_state ();
      end_action (false, -1);
    }
  }

  end_action (false, -1);
}

/* mount
   -----
   To mount, we must:
   (1)  Convert the path to a vnode A.
   (2)  Bind to the file system.  Let B be the vnode representing the root of its file system.
   (3)  Insert an entry in the mount table that causes future path traversals to substitute A -> B.
   Step (1) is started in the decode action.
   Steps (2) and (3) are completed in this action.

   Pre:  the internal state machine is loaded (client_request_bd != -1) with a mount request (client_request_type == MOUNT) and the path name lookup has finished (path_lookup_done == true)
   Post: The internal state machine will be empty (client_request_bd == -1)
 */
static bool
mount_precondition (void)
{
  return client_request_bd != -1 && client_request_type == VFS_MOUNT && path_lookup_done && vfs_aid != -1;
}

BEGIN_INTERNAL (NO_PARAMETER, MOUNT_NO, "", "", mount, int param)
{
  ssyslog ("vfs: mount\n");
  initialize ();
  scheduler_remove (MOUNT_NO, param);
  if (mount_precondition ()) {

    /* The path could not be translated. */
    if (path_lookup_error) {
      form_mount_response (VFS_PATH_DNE);
      reset_client_state ();
      end_action (false, -1);
    }

    /* The final destination was not a directory. */
    if (path_lookup_current.node.type != DIRECTORY) {
      form_mount_response (VFS_NOT_DIRECTORY);
      reset_client_state ();
      end_action (false, -1);
    }

    /* Bind to the file system. */
    description_t desc;
    if (description_init (&desc, mount_aid) == -1) {
      form_mount_response (VFS_AID_DNE);
      reset_client_state ();
      end_action (false, -1);
    }

    const ano_t request = description_name_to_number (&desc, VFS_FS_REQUEST_NAME);
    const ano_t response = description_name_to_number (&desc, VFS_FS_RESPONSE_NAME);

    description_fini (&desc);

    if (request == NO_ACTION ||
	response == NO_ACTION) {
      form_mount_response (VFS_NOT_FS);
      reset_client_state ();
      end_action (false, -1);
    }

    /* Bind to the response first so they don't get lost. */
    if (bind (mount_aid, response, 0, vfs_aid, VFS_FS_RESPONSE_NO, 0) == -1 ||
	bind (vfs_aid, VFS_FS_REQUEST_NO, 0, mount_aid, request, 0) == -1) {
      /* If binding fails, it is because the file system's actions are available. */
      /* TODO:  Unbind. */
      form_mount_response (VFS_NOT_AVAILABLE);
      reset_client_state ();
      end_action (false, -1);
    }

    /* The mount succeeded.  Insert an entry into the list. */
    vnode_t b;
    b.node.type = DIRECTORY;
    b.node.id = 0;
    b.aid = mount_aid;
    mount_item_t* item = mount_item_create (&path_lookup_current, &b);
    item->next = mount_head;
    mount_head = item;

    form_mount_response (VFS_SUCCESS);
    reset_client_state ();
  }

  end_action (false, -1);
}

/* readfile
   -----
   To readfile, we must:
   (1)  Convert the path to a vnode.
   (2)  Request the file.
   (3)  Forward the response.
   Step (1) is started in the decode action.
   The request is prepared in this action.
   The response is forwared when a response is received.

   Pre:  The client has made a request and the request is to read a file and the path lookup is done and the buffer to send a message to a file system is available and we have not sent the request
   Post: fs_bd != -1 && readfile_sent == true
 */
static bool
readfile_request_precondition (void)
{
  return client_request_bd != -1 && client_request_type == VFS_READFILE && path_lookup_done && fs_bd == -1 && readfile_sent == false;
}

BEGIN_INTERNAL (NO_PARAMETER, READFILE_REQUEST_NO, "", "", readfile_request, int param)
{
  ssyslog ("vfs: readfile_request\n");
  initialize ();
  scheduler_remove (READFILE_REQUEST_NO, param);

  if (readfile_request_precondition ()) {
    /* The path could not be translated. */
    if (path_lookup_error) {
      form_readfile_response (VFS_PATH_DNE);
      reset_client_state ();
      end_action (false, -1);
    }

    /* The final destination was not a file. */
    if (path_lookup_current.node.type != FILE) {
      form_readfile_response (VFS_NOT_FILE);
      reset_client_state ();
      end_action (false, -1);
    }

    /* Form a message to a file system. */
    fs_aid = path_lookup_current.aid;
    fs_bd = write_vfs_fs_readfile_request (path_lookup_current.node.id);
    fs_type = VFS_FS_READFILE;

    readfile_sent = true;
  }

  end_action (false, -1);
}

static void
end_action (bool output_fired,
	    bd_t bd)
{
  if (bd != -1) {
    buffer_queue_push (&destroy_queue, 0, bd);
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
  if (mount_precondition ()) {
    scheduler_add (MOUNT_NO, 0);
  }
  if (readfile_request_precondition ()) {
    scheduler_add (READFILE_REQUEST_NO, 0);
  }


  scheduler_finish (output_fired, bd);
}
