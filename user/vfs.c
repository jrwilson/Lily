#include "vfs_msg.h"
#include <automaton.h>
#include <string.h>
#include <buffer_file.h>
#include <description.h>
#include <dymem.h>
#include <callback_queue.h>
#include "syslog.h"

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
  TODO:  We are vunerable to circular mounts.
  TODO:  Subscribe to the client.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define INIT_NO 1
#define STOP_NO 2
#define SYSLOG_NO 3
#define VFS_REQUEST_NO 4
#define VFS_RESPONSE_NO 5
#define VFS_FS_REQUEST_NO 6
#define VFS_FS_RESPONSE_NO 7

#define WARNING "vfs: warning: "
#define INFO "vfs: info: "

/* Our aid. */
static aid_t vfs_aid = -1;

/* Initialization flag. */
static bool initialized = false;

typedef enum {
  RUN,
  STOP,
} state_t;
static state_t state = RUN;

/* Syslog buffer. */
static bd_t syslog_bd = -1;
static buffer_file_t syslog_buffer;

/*
  Begin Path Section
  ==================
*/

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

/*
  End Path Section
  ================
*/

/*
  Begin File System Section
  ========================
*/

typedef struct file_system file_system_t;
struct file_system {
  aid_t aid;
  vfs_fs_request_queue_t request_queue;
  callback_queue_t callback_queue;
  bd_t request_bda;
  bd_t request_bdb;
  file_system_t* global_next;
  bool on_request_queue;
  file_system_t* request_next;
};

/* List of file systems. */
static file_system_t* file_system_head = 0;
/* File system request queue. */
static file_system_t* request_head = 0;
static file_system_t** request_tail = &request_head;

static file_system_t*
file_system_create (aid_t aid)
{
  bd_t request_bda = buffer_create (0);
  if (request_bda == -1) {
    return 0;
  }
  bd_t request_bdb = buffer_create (0);
  if (request_bdb == -1) {
    buffer_destroy (request_bda);
    return 0;
  }

  file_system_t* fs = malloc (sizeof (file_system_t));
  memset (fs, 0, sizeof (file_system_t));
  fs->aid = aid;
  vfs_fs_request_queue_init (&fs->request_queue);
  callback_queue_init (&fs->callback_queue);
  fs->request_bda = request_bda;
  fs->request_bdb = request_bdb;
  fs->global_next = file_system_head;
  file_system_head = fs;

  bfprintf (&syslog_buffer, INFO "TODO:  subscribe to file system\n");

  return fs;
}

static void
file_system_put_on_request_queue (file_system_t* fs)
{
  if (!fs->on_request_queue && !vfs_fs_request_queue_empty (&fs->request_queue)) {
    fs->on_request_queue = true;
    fs->request_next = 0;
    *request_tail = fs;
    request_tail = &fs->request_next;
  }
}

static file_system_t*
find_file_system (aid_t aid)
{
  file_system_t* fs;
  for (fs = file_system_head; fs != 0; fs = fs->global_next) {
    if (fs->aid == aid) {
      break;
    }
  }
  return fs;
}

static file_system_t**
find_file_system_request (aid_t aid)
{
  file_system_t** fs;
  for (fs = &request_head; *fs != 0; fs = &(*fs)->request_next) {
    if ((*fs)->aid == aid) {
      break;
    }
  }
  return fs;
}

static void
file_system_pop_request (file_system_t** f)
{
  file_system_t* fs = *f;
  /* Pop the response. */
  if (vfs_fs_request_queue_pop_to_buffer (&fs->request_queue, fs->request_bda, fs->request_bdb) != 0) {
    bfprintf (&syslog_buffer, WARNING "could not write to file system request buffer\n");
  }

  /* Remove the file system from the request queue. */
  *f = fs->request_next;
  if (request_head == 0) {
    request_tail = &request_head;
  }
  fs->on_request_queue = false;

  /* Add the file system to the request queue. */
  file_system_put_on_request_queue (fs);
}

/*
  End File System Section
  =======================
*/

/*
  Begin Vnode Section
  ===================
*/

/* A virtual node associates an node with a file system. */
typedef struct {
  file_system_t* fs;	/* The filesystem that contains the node. */
  vfs_fs_node_t node;
} vnode_t;

static bool
vnode_equal (const vnode_t* x,
	     const vnode_t* y)
{
  return x->fs == y->fs && x->node.id == y->node.id;
}

/*
  End Vnode Section
  =================
*/

/*
  Begin Mount Section
  ===================
*/

typedef struct mount_item mount_item_t;
struct mount_item {
  vnode_t a;
  vnode_t b;
  mount_item_t* next;
};

/* The virtual root. */
static vnode_t root;

/* Table to translate vnodes based on mounting. */
static mount_item_t* mount_head = 0;

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

/*
  End Mount Section
  =================
*/

/*
  Begin Client Section
  ====================
*/

typedef struct client client_t;
struct client {
  aid_t aid;
  vfs_request_queue_t request_queue;
  vfs_response_queue_t response_queue;
  bd_t response_bda;
  bd_t response_bdb;
  client_t* global_next;
  bool on_response_queue;
  client_t* response_next;

  /* Variables for looking up paths. */
  const char* path_lookup_path;
  size_t path_lookup_path_size;
  vnode_t path_lookup_current;
  const char* path_lookup_begin;
  const char* path_lookup_end;
};

static client_t* client_head = 0;
static client_t* response_head = 0;
static client_t** response_tail = &response_head;

static client_t*
find_client (aid_t aid)
{
  client_t* client;
  for (client = client_head; client != 0; client = client->global_next) {
    if (client->aid == aid) {
      break;
    }
  }

  return client;
}

static client_t**
find_client_response (aid_t aid)
{
  client_t** client;
  for (client = &response_head; *client != 0; client = &(*client)->response_next) {
    if ((*client)->aid == aid) {
      break;
    }
  }

  return client;
}

static client_t*
create_client (aid_t aid)
{
  bd_t response_bda = buffer_create (0);
  if (response_bda == -1) {
    return 0;
  }
  bd_t response_bdb = buffer_create (0);
  if (response_bdb == -1) {
    buffer_destroy (response_bda);
    return 0;
  }

  client_t* client = malloc (sizeof (client_t));
  memset (client, 0, sizeof (client_t));
  client->aid = aid;
  vfs_request_queue_init (&client->request_queue);
  vfs_response_queue_init (&client->response_queue);
  client->response_bda = response_bda;
  client->response_bdb = response_bdb;
  client->global_next = client_head;
  client_head = client;
  client->on_response_queue = false;
  
  bfprintf (&syslog_buffer, INFO "TODO:  Subscribe to client\n");

  return client;
}

static void
client_put_on_response_queue (client_t* client)
{
  if (!client->on_response_queue && !vfs_response_queue_empty (&client->response_queue)) {
    client->on_response_queue = true;
    client->response_next = 0;
    *response_tail = client;
    response_tail = &client->response_next;
  }
}

static void
client_initialize_path_lookup (client_t* client,
			       const char* path,
			       size_t path_size)
{
  client->path_lookup_path = path;
  client->path_lookup_path_size = path_size;
  client->path_lookup_end = client->path_lookup_path;
}

static void
client_start (client_t* client);

static void
client_answer (client_t* client)
{
  client_put_on_response_queue (client);
  vfs_request_queue_pop (&client->request_queue);
  client_start (client);
}

static void
readfile_callback (void* data,
		   bd_t bda,
		   bd_t bdb)
{
  client_t* client = data;

  vfs_fs_error_t error;
  size_t size;
  if (read_vfs_fs_readfile_response (bda, bdb, &error, &size) != 0) {
    bfprintf (&syslog_buffer, WARNING "could not read readfile response\n");
    return;
  }

  if (error != VFS_FS_SUCCESS) {
    bfprintf (&syslog_buffer, WARNING "readfile did not succeed\n");
    return;
  }
      
  if (size > buffer_size (bdb) * pagesize ()) {
    bfprintf (&syslog_buffer, WARNING "readfile response has bad size\n");
    return;
  }

  /* Answer. */
  if (vfs_response_queue_push_readfile (&client->response_queue, VFS_SUCCESS, size, bdb) != 0) {
    bfprintf (&syslog_buffer, WARNING "could not copy file buffer\n");
    return;
  }
  client_answer (client);
}

static void
client_path_lookup_done (client_t* client,
			 bool fail)
{
  const vfs_request_queue_item_t* req = vfs_request_queue_front (&client->request_queue);
  switch (req->type) {
  case VFS_UNKNOWN:
    bfprintf (&syslog_buffer, WARNING "unknown request in client request queue\n");
    break;
  case VFS_MOUNT:
    /* The path could not be translated. */
    if (fail) {
      /* Answer. */
      vfs_response_queue_push_mount (&client->response_queue, VFS_PATH_DNE);
      client_answer (client);
      return;
    }

    /* The final destination was not a directory. */
    if (client->path_lookup_current.node.type != DIRECTORY) {
      /* Answer. */
      vfs_response_queue_push_mount (&client->response_queue, VFS_NOT_DIRECTORY);
      client_answer (client);
      return;
    }

    /* Bind to the file system. */
    description_t desc;
    if (description_init (&desc, req->u.mount.aid) != 0) {
      /* Answer. */
      vfs_response_queue_push_mount (&client->response_queue, VFS_AID_DNE);
      client_answer (client);
      return;
    }

    const ano_t request = description_name_to_number (&desc, VFS_FS_REQUEST_NAME, strlen (VFS_FS_REQUEST_NAME) + 1);
    const ano_t response = description_name_to_number (&desc, VFS_FS_RESPONSE_NAME, strlen (VFS_FS_RESPONSE_NAME) + 1);

    description_fini (&desc);

    if (request == NO_ACTION ||
    	response == NO_ACTION) {
      /* Answer. */
      vfs_response_queue_push_mount (&client->response_queue, VFS_NOT_FS);
      client_answer (client);
      return;
    }

    /* Bind to the response first so they don't get lost. */
    bid_t bid = bind (req->u.mount.aid, response, 0, vfs_aid, VFS_FS_RESPONSE_NO, 0);
    if (bid == -1) {
      /* Answer. */
      vfs_response_queue_push_mount (&client->response_queue, VFS_NOT_AVAILABLE);
      client_answer (client);
      return;
    }

    if (bind (vfs_aid, VFS_FS_REQUEST_NO, 0, req->u.mount.aid, request, 0) == -1) {
      unbind (bid);
      /* Answer. */
      vfs_response_queue_push_mount (&client->response_queue, VFS_NOT_AVAILABLE);
      client_answer (client);
      return;
    }

    /* The mount succeeded.  Insert an entry into the list. */
    file_system_t* fs = file_system_create (req->u.mount.aid);
    if (fs == 0) {
      // TODO
    }

    vnode_t b;
    b.fs = fs;
    b.node.type = DIRECTORY;
    b.node.id = 0;
    mount_item_t* item = mount_item_create (&client->path_lookup_current, &b);
    item->next = mount_head;
    mount_head = item;

    /* Answer. */
    vfs_response_queue_push_mount (&client->response_queue, VFS_SUCCESS);
    client_answer (client);
    return;

    break;
  case VFS_READFILE:
    /* The path could not be translated. */
    if (fail) {
      /* Answer. */
      vfs_response_queue_push_readfile (&client->response_queue, VFS_PATH_DNE, 0, -1);
      client_answer (client);
      return;
    }

    /* The final destination was not a file. */
    if (client->path_lookup_current.node.type != FILE) {
      /* Answer. */
      vfs_response_queue_push_readfile (&client->response_queue, VFS_NOT_FILE, 0, -1);
      client_answer (client);
      return;
    }

    /* Form a message to a file system. */
    vfs_fs_request_queue_push_readfile (&client->path_lookup_current.fs->request_queue, client->path_lookup_current.node.id);
    callback_queue_push (&client->path_lookup_current.fs->callback_queue, readfile_callback, client);
    file_system_put_on_request_queue (client->path_lookup_current.fs);
    return;

    break;
  }
}

static void
client_resume_path_lookup (client_t* client,
			   vnode_t node);

static void
descend_callback (void* data,
		  bd_t bda,
		  bd_t bdb)
{
  client_t* client = data;

  vfs_fs_error_t error;
  vfs_fs_node_t node;
  if (read_vfs_fs_descend_response (bda, bdb, &error, &node) != 0) {
    bfprintf (&syslog_buffer, WARNING "could not read descend response\n");
    return;
  }

  if (error != VFS_FS_SUCCESS) {
    client_path_lookup_done (client, true);
    return;
  }

  client->path_lookup_current.node = node;
  client_resume_path_lookup (client, client->path_lookup_current);
}

static void
client_resume_path_lookup (client_t* client,
			   vnode_t node)
{
  client->path_lookup_current = node;

  /* First, look for an entry in the mount table. */
  mount_item_t* item = mount_head;
  while (item != 0) {
    if (vnode_equal (&item->a, &client->path_lookup_current)) {
      /* Found an entry. Translate and start over. */
      client->path_lookup_current = item->b;
      item = mount_head;
    }
    else {
      item = item->next;
    }
  }
  
  if (*(client->path_lookup_end) == 0) {
    /* We are at the end of the path. */
    client_path_lookup_done (client, false);
    return;
  }
  
  /* Advance to the next element in the path. */
  client->path_lookup_begin = client->path_lookup_end + 1;
  
  if (*(client->path_lookup_begin) == 0) {
    /* We are at the end of the path. */
    client_path_lookup_done (client, false);
    return;
  }
  
  /* Find the terminator (0) or next path separator (/). */
  client->path_lookup_end = client->path_lookup_begin + 1;
  while (*(client->path_lookup_end) != 0 && *(client->path_lookup_end) != '/') {
    ++client->path_lookup_end;
  }
  
  /* Ensure that the path search can continue. */
  if (client->path_lookup_current.fs == 0 || client->path_lookup_current.node.type != DIRECTORY) {
    /* No file system or not a directory. */
    client_path_lookup_done (client, true);
    return;
  }
  
  /* Form a message to a file system. */
  vfs_fs_request_queue_push_descend (&client->path_lookup_current.fs->request_queue, client->path_lookup_current.node.id, client->path_lookup_begin, client->path_lookup_end - client->path_lookup_begin);
  callback_queue_push (&client->path_lookup_current.fs->callback_queue, descend_callback, client);
  file_system_put_on_request_queue (client->path_lookup_current.fs);
}

static void
client_start (client_t* client)
{
  if (!vfs_request_queue_empty (&client->request_queue)) {
    /* The request queue was empty but now is not.  Start processing the request. */
    const vfs_request_queue_item_t* req = vfs_request_queue_front (&client->request_queue);
    switch (req->type) {
    case VFS_UNKNOWN:
      /* An unknown request type got on the queue.  This is a logic error. */
      bfprintf (&syslog_buffer, WARNING "unknown request on client request queue\n");
      break;
    case VFS_MOUNT:
      if (!check_absolute_path (req->u.mount.path, req->u.mount.path_size)) {
	/* Answer. */
	vfs_response_queue_push_mount (&client->response_queue, VFS_BAD_PATH);
	client_answer (client);
	return;
      }
    
      /* See if the file system is already mounted.
	 If it is, its an error.
      */
      mount_item_t* mnt;
      for (mnt = mount_head; mnt != 0; mnt = mnt->next) {
	if (mnt->b.fs->aid == req->u.mount.aid) {
	  /* Answer. */
	  vfs_response_queue_push_mount (&client->response_queue, VFS_ALREADY_MOUNTED);
	  client_answer (client);
	  return;
	}
      }
    
      /* Start the process of converting the path to a vnode starting at the root. */
      client_initialize_path_lookup (client, req->u.mount.path, req->u.mount.path_size);
      client_resume_path_lookup (client, root);
      break;
    case VFS_READFILE:
      if (!check_absolute_path (req->u.readfile.path, req->u.readfile.path_size)) {
	/* Answer. */
	vfs_response_queue_push_readfile (&client->response_queue, VFS_BAD_PATH, 0, -1);
	client_answer (client);
	return;
      }

      /* Start the process of converting the path to a vnode starting at the root. */
      client_initialize_path_lookup (client, req->u.readfile.path, req->u.readfile.path_size);
      client_resume_path_lookup (client, root);
      break;
    }
  }
}

static void
client_push_request (client_t* client,
		     bd_t bda,
		     bd_t bdb)
{
  vfs_type_t type;
  bool empty = vfs_request_queue_empty (&client->request_queue);
  if (vfs_request_queue_push_from_buffer (&client->request_queue, bda, bdb, &type) == 0) {
    if (empty) {
      client_start (client);
    }
  }
  else {
    /* Bad request. */
    vfs_response_queue_push_bad_request (&client->response_queue, type);
    /* Add to the response queue if not already there. */
    client_put_on_response_queue (client);
  }
}

static void
client_pop_response (client_t** c)
{
  client_t* client = *c;
  /* Pop the response. */
  if (vfs_response_queue_pop_to_buffer (&client->response_queue, client->response_bda, client->response_bdb) != 0) {
    bfprintf (&syslog_buffer, WARNING "could not write to client response buffer\n");
  }

  /* Remove the client from the response queue. */
  *c = client->response_next;
  if (response_head == 0) {
    response_tail = &response_head;
  }
  client->on_response_queue = false;

  /* Add the client to the response queue. */
  client_put_on_response_queue (client);
}

/*
  End Client Section
  ==================
*/

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    syslog_bd = buffer_create (0);
    if (syslog_bd == -1) {
      /* Nothing we can do. */
      exit ();
    }
    if (buffer_file_initw (&syslog_buffer, syslog_bd) != 0) {
      /* Nothing we can do. */
      exit ();
    }

    aid_t syslog_aid = lookup (SYSLOG_NAME, strlen (SYSLOG_NAME) + 1);
    if (syslog_aid != -1) {
      /* Bind to the syslog. */

      description_t syslog_description;
      if (description_init (&syslog_description, syslog_aid) != 0) {
	exit ();
      }
      
      const ano_t syslog_stdin = description_name_to_number (&syslog_description, SYSLOG_STDIN, strlen (SYSLOG_STDIN) + 1);
      
      description_fini (&syslog_description);
      
      /* We bind the response first so they don't get lost. */
      if (bind (getaid (), SYSLOG_NO, 0, syslog_aid, syslog_stdin, 0) == -1) {
	exit ();
      }
    }

    root.fs = 0;
    root.node.type = DIRECTORY;
    root.node.id = 0;

    vfs_aid = getaid ();
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

/* stop
   ----
   Stop the automaton.
   
   Pre:  state == STOP and syslog_buffer is empty
   Post: 
*/
static bool
stop_precondition (void)
{
  return state == STOP && buffer_file_size (&syslog_buffer) == 0;
}

BEGIN_INTERNAL (NO_PARAMETER, STOP_NO, "", "", stop, ano_t ano, int param)
{
  initialize ();

  if (stop_precondition ()) {
    exit ();
  }
  finish_internal ();
}

/* syslog
   ------
   Output error messages.
   
   Pre:  syslog_buffer is not empty
   Post: syslog_buffer is empty
*/
static bool
syslog_precondition (void)
{
  return buffer_file_size (&syslog_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, SYSLOG_NO, "", "", syslogx, ano_t ano, int param)
{
  initialize ();

  if (syslog_precondition ()) {
    buffer_file_truncate (&syslog_buffer);
    finish_output (true, syslog_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

/* client_request
   --------------
   Receive a request from a client.

   Post: the request is added to the request queue of the client or a response is added to the response queue of the client
 */
BEGIN_INPUT (AUTO_PARAMETER, VFS_REQUEST_NO, VFS_REQUEST_NAME, "", client_request, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  client_t* client = find_client (aid);
  if (client == 0) {
    client = create_client (aid);
    if (client == 0) {
      bfprintf (&syslog_buffer, WARNING "could not create client\n");
      finish_input (bda, bdb);
    }
  }
  client_push_request (client, bda, bdb);
  finish_input (bda, bdb);
}

/* client_response
   ---------------
   Send a response to a client.

   Pre:  The client exists and has a response.
   Post: The first response for the client is removed.
 */
BEGIN_OUTPUT (AUTO_PARAMETER, VFS_RESPONSE_NO, VFS_RESPONSE_NAME, "", client_response, ano_t ano, aid_t aid)
{
  initialize ();

  /* Find the client on the response queue. */
  client_t** c = find_client_response (aid);
  client_t* client = *c;
  if (client != 0) {
    client_pop_response (c);
    finish_output (true, client->response_bda, client->response_bdb);
  }
  else {
    finish_output (false, -1, -1);
  }
}

/* file_system_request
   -------------------
   Send a request to a file system.

   Pre:  ???
   Post: ???
 */
BEGIN_OUTPUT (AUTO_PARAMETER, VFS_FS_REQUEST_NO, "", "", file_system_request, ano_t ano, aid_t aid)
{
  initialize ();

  /* Find the file system on the request queue. */
  file_system_t** f = find_file_system_request (aid);
  file_system_t* fs = *f;
  if (fs != 0) {
    file_system_pop_request (f);
    finish_output (true, fs->request_bda, fs->request_bdb);
  }
  else {
    finish_output (false, -1, -1);
  }
}

/* file_system_response
   --------------------
   Process a response from a file system.

   Post:
 */
BEGIN_INPUT (AUTO_PARAMETER, VFS_FS_RESPONSE_NO, "", "", file_system_response, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  file_system_t* fs = find_file_system (aid);

  if (callback_queue_empty (&fs->callback_queue)) {
    /* The file system produced a response when one was not requested. */
    finish_input (bda, bdb);
  }

  const callback_queue_item_t* item = callback_queue_front (&fs->callback_queue);
  callback_t callback = callback_queue_item_callback (item);
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&fs->callback_queue);

  callback (data, bda, bdb);

  finish_input (bda, bdb);
}

void
do_schedule (void)
{
  if (stop_precondition ()) {
    schedule (STOP_NO, 0);
  }
  if (syslog_precondition ()) {
    schedule (SYSLOG_NO, 0);
  }
  if (response_head != 0) {
    schedule (VFS_RESPONSE_NO, response_head->aid);
  }
  if (request_head != 0) {
    schedule (VFS_FS_REQUEST_NO, request_head->aid);
  }
}
