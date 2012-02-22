#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <dymem.h>
#include <buffer_file.h>
#include <buffer_queue.h>
#include "cpio.h"
#include "vfs_msg.h"

/*
  Tmpfs
  =====
  Tmpfs is an in-memory file system based on the tmpfs system of Linux.
  Tmpfs is initialized with a cpio archive that contains the initial contents of the file system.
  Currently, only files and directories are supported.

  Tmpfs is designed to work with a virtual file system.
  All of the operations in tmpfs involve inodes which correspond to files, directories, etc.
  One reads and writes data by specifying the operation, the file's inode, and the data.
  One can traverse paths by specifying a directory's inode and the next element of the path to determine the next inode.

  Protocol
  --------
  I considered three possible protocols for tmpfs.
  The first is a simple request-response protocol.
  The second is also a request-response protocol but bulk data follows a corresponding request or response.
  The third is a request-response protocol with an additional channel for bulk data transfers.

  The potential problems of the first design is that it requires buffer manipulation and can't take advantage of auto-mapping.
  For example, to write data to a file, one must create a buffer containing the request and then append a buffer containing the data.
  Since the data may be large, the tmpfs automaton should not auto-map it but rather extract a buffer containing the request and map it.
  The advantage of the first design is that it results in a stateless protocol which is considerably easier to implement.

  The second design solves the buffer manipulation problem but introduces state.
  Since the request and data are sent separately, there is no need to pack and unpack them.
  However, the auto-mapping problem remains as the data can be large and arrives at the same input.
  In the second design, one must remember the last request to determine if data is expected or not.

  The third design solves both the buffer manipulation problem and auto-mapping problem but is more complex than the second design.
  In the second design, all messages can be placed in a queue and processed in FIFO order.
  The third design introduces additional overhead to synchronize the request/response queues and the data queues.
  The problem is summarized by saying that the data cannot be placed on an outgoing queue until the corresponding request/response has been sent.

  To make an informed decision, one needs estimates of the system call overhead for the first design which can be compared to the expected overhead of the second and third design.
  Since I currently lack this information, I will go with the first design since it is easier.

  Implementation Details
  ----------------------
  The main data structure is the inode.
  Each inode has an id, a type, and a name.
  File inodes have a buffer.
  All inodes have a link to their parent, the first child, and their next sibling.
  This allows them to be organized into a tree structure.
  Other organizations are possible, e.g., each nodes contains an array listing its children.
  The top-level inode is called the root.

  The other important data structure is the inode list which is a vector that allows one to lookup an inode given its id.
  Thus, the first part of most requests will involve a table look-up to translate the given id into an inode.
  
  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define TMPFS_REQUEST_NO 1
#define TMPFS_RESPONSE_NO 2
#define DESTROY_BUFFERS_NO 3

/* Every inode in the filesystem is either a file or directory. */
typedef enum {
  FILE,
  DIRECTORY,
} inode_type_t;

typedef struct inode_struct inode_t;
struct inode_struct {
  size_t id;
  inode_type_t type;
  size_t name_size;
  char* name;
  bd_t bd;
  size_t bd_size;
  size_t size;
  inode_t* parent;
  inode_t* first_child;
  inode_t* next_sibling;
};

/* A vector that maps id to inode. */
static size_t nodes_capacity = 0;
static size_t nodes_size = 0;
static inode_t** nodes = 0;

/* The root. */
static inode_t* root = 0;

/* A list of free inodes. */
static inode_t* free_list = 0;

/* Initialization flag. */
static bool initialized = false;

/* Queue for responses. */
static buffer_queue_t response_queue;

/* Queue of buffers to destroy. */
static buffer_queue_t destroy_queue;

static inode_t*
inode_create (inode_type_t type,
	     const char* name,
	     size_t name_size,
	     bd_t bd,
	     size_t bd_size,
	     size_t size,
	     inode_t* parent)
{
  inode_t* node;

  if (free_list != 0) {
    /* Take a node from the free list. */
    node = free_list;
    free_list = node->next_sibling;
  }
  else {
    /* Expand the vector if necessary. */
    if (nodes_size == nodes_capacity) {
      nodes_capacity *= 2;
      nodes = realloc (nodes, nodes_capacity * sizeof (inode_t*));
    }

    /* Create a new node. */
    node = malloc (sizeof (inode_t));
    node->id = nodes_size++;
  }

  /* Place the node in the index. */
  nodes[node->id] = node;

  /* Initialize the node. */
  node->type = type;
  node->name_size = name_size;
  node->name = malloc (name_size);
  memcpy (node->name, name, name_size);
  node->bd = bd;
  node->bd_size = bd_size;
  node->size = size;
  node->parent = parent;
  node->first_child = 0;
  node->next_sibling = 0;

  return node;
}

static inode_t*
directory_find_or_create (inode_t* parent,
			  const char* name,
			  size_t name_size)
{
  /* Iterate over the parent's children looking for the name. */
  inode_t** ptr;
  for (ptr = &parent->first_child; *ptr != 0; ptr = &(*ptr)->next_sibling) {
    if ((*ptr)->name_size == name_size &&
  	memcmp ((*ptr)->name, name, name_size) == 0) {
      return *ptr;
    }
  }

  /* Create a node. */
  *ptr = inode_create (DIRECTORY, name, name_size, -1, 0, 0, parent);

  return *ptr;
}

static inode_t*
file_find_or_create (inode_t* parent,
		     const char* name,
		     size_t name_size,
		     bd_t bd,
		     size_t bd_size,
		     size_t size)
{
  /* Iterate over the parent's children looking for the name. */
  inode_t** ptr;
  for (ptr = &parent->first_child; *ptr != 0; ptr = &(*ptr)->next_sibling) {
    if ((*ptr)->name_size == name_size &&
  	memcmp ((*ptr)->name, name, name_size) == 0) {
      return *ptr;
    }
  }

  /* Create a node. */
  *ptr = inode_create (FILE, name, name_size, buffer_copy (bd, 0, bd_size), bd_size, size, parent);

  return *ptr;
}

static void
print (inode_t* node)
{
  if (node != 0) {
    /* Print this node. */
    switch (node->type) {
    case FILE:
      {
	const char* s = "FILE ";
	syslog (s, strlen (s));
      }
      break;
    case DIRECTORY:
      {
	const char* s = "DIRECTORY ";
	syslog (s, strlen (s));
      }
      break;
    }
    syslog (node->name, node->name_size);
    syslog ("\n", 1);

    /* Print the children. */
    for (inode_t* child = node->first_child; child != 0; child = child->next_sibling) {
      print (child);
    }
  }
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    /* Allocate the inode vector. */
    nodes_capacity = 1;
    nodes_size = 0;
    nodes = malloc (nodes_capacity * sizeof (inode_t*));
    
    /* Create the root. */
    root = inode_create (DIRECTORY, "", 0, -1, 0, 0, 0);
    /* TODO:  Do we need to make the root its own parent? */

    buffer_queue_init (&response_queue);
    buffer_queue_init (&destroy_queue);
  }
}

static void
form_response (aid_t aid,
	       vfs_fs_type_t type,
	       vfs_fs_error_t error,
	       bd_t request_bd)
{
  /* Create a response. */
  size_t bd_size = size_to_pages (sizeof (vfs_fs_response_t));
  bd_t bd = buffer_create (bd_size);
  vfs_fs_response_t* rr = buffer_map (bd);
  rr->type = type;
  rr->error = error;
  /* We don't unmap it because we are going to destroy it when we respond. */
  buffer_queue_push (&response_queue, aid, bd, bd_size);

  if (request_bd != -1) {
    buffer_destroy (request_bd);
  }
}

static void
process_request (aid_t aid,
		 bd_t bd,
		 size_t bd_size)
{
  if (bd == -1) {
    form_response (aid, VFS_FS_UNKNOWN, VFS_FS_NO_BUFFER, -1);
    return;
  }

  /* Create a buffer that contains the request. */
  size_t request_bd_size = size_to_pages (sizeof (vfs_fs_request_t));
  bd_t request_bd = buffer_copy (bd, 0, request_bd_size);
  if (request_bd == -1) {
    form_response (aid, VFS_FS_UNKNOWN, VFS_FS_NO_MAP, -1);
    return;
  }

  buffer_file_t file;
  if (buffer_file_open (&file, request_bd, request_bd_size, false) == -1) {
    form_response (aid, VFS_FS_UNKNOWN, VFS_FS_NO_MAP, request_bd);
    return;
  }

  const vfs_fs_request_t* r = buffer_file_readp (&file, sizeof (vfs_fs_request_t));
  if (r == 0) {
    form_response (aid, VFS_FS_UNKNOWN, VFS_FS_BAD_REQUEST, request_bd);
    return;
  }

  switch (r->type) {
  case VFS_FS_READ_FILE:
    {
      if (r->u.read_file.inode >= nodes_size) {
	form_response (aid, VFS_FS_READ_FILE, VFS_FS_BAD_INODE, request_bd);
	return;
      }

      inode_t* inode = nodes[r->u.read_file.inode];

      if (inode == 0) {
	form_response (aid, VFS_FS_READ_FILE, VFS_FS_BAD_INODE, request_bd);
	return;
      }

      /* Create a response. */
      size_t bd_size = size_to_pages (sizeof (vfs_fs_response_t));
      bd_t bd = buffer_create (bd_size);
      vfs_fs_response_t* rr = buffer_map (bd);
      rr->type = VFS_FS_READ_FILE;
      rr->error = VFS_FS_SUCCESS;
      rr->u.read_file.size = inode->size;
      buffer_unmap (bd);
      /* Append the file. */
      bd_size += inode->bd_size;
      buffer_append (bd, inode->bd, 0, inode->bd_size);

      /* Enqueue the response. */
      buffer_queue_push (&response_queue, aid, bd, bd_size);

      buffer_destroy (request_bd);

      return;
    }
    break;
  default:
    form_response (aid, VFS_FS_UNKNOWN, VFS_FS_BAD_REQUEST, request_bd);
    return;
  }
}

static void
schedule (void);

BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t aid, bd_t bd, size_t bd_size)
{
  {
    const char* s = "tmpfs: init\n";
    syslog (s, strlen (s));
  }

  initialize ();

  /* Parse the cpio archive looking for files that we need. */
  cpio_archive_t archive;
  if (cpio_archive_init (&archive, bd, bd_size) == 0) {

    cpio_file_t* file;
    while ((file = cpio_archive_next_file (&archive)) != 0) {

      /* Ignore the "." directory. */
      if (strcmp (file->name, ".") != 0) {
	
      	inode_t* parent = root;
	
  	const char* begin;
      	for (begin = file->name; begin < file->name + file->name_size;) {
      	  /* Find the delimiter. */
      	  const char* end = memchr (begin, '/', file->name_size - (begin - file->name));
	  
      	  if (end != 0) {
      	    /* Process part of the path. */
      	    parent = directory_find_or_create (parent, begin, end - begin);
      	    /* Update the beginning. */
      	    begin = end + 1;
      	  }
      	  else {
      	    break;
      	  }
      	}
	
      	switch (file->mode & CPIO_TYPE_MASK) {
      	case CPIO_REGULAR:
  	  file_find_or_create (parent, begin, file->name_size - 1 - (begin - file->name), file->bd, file->bd_size, file->size);
      	  break;
      	case CPIO_DIRECTORY:
      	  directory_find_or_create (parent, begin, file->name_size - 1 - (begin - file->name));
      	  break;
      	}
      }

      /* Destroy the cpio file. */
      cpio_file_destroy (file);
    }

    /* TODO */
    print (root);
  }

  buffer_destroy (bd);

  schedule ();
  scheduler_finish (false, -1);
}

BEGIN_INPUT (AUTO_PARAMETER, TMPFS_REQUEST_NO, VFS_FS_REQUEST_NAME, "", request, aid_t aid, bd_t bd, size_t bd_size)
{
  {
    const char* s = "tmpfs: request\n";
    syslog (s, strlen (s));
  }

  initialize ();
  process_request (aid, bd, bd_size);
  buffer_destroy (bd);
  schedule ();
  scheduler_finish (false, -1);
}

BEGIN_OUTPUT (AUTO_PARAMETER, TMPFS_RESPONSE_NO, VFS_FS_RESPONSE_NAME, "", response, aid_t aid, size_t bc)
{
  {
    const char* s = "tmpfs: response\n";
    syslog (s, strlen (s));
  }

  initialize ();
  scheduler_remove (TMPFS_RESPONSE_NO, aid);

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&response_queue, aid);

  if (item != 0) {
    /* Found a response.  Execute. */
    bd_t bd = buffer_queue_item_bd (item);
    size_t bd_size = buffer_queue_item_size (item);
    buffer_queue_erase (&response_queue, item);

    buffer_queue_push (&destroy_queue, 0, bd, bd_size);

    schedule ();
    scheduler_finish (true, bd);
  }
  else {
    /* Did not find a response. */
    schedule ();
    scheduler_finish (false, -1);
  }
}

static bool
destroy_buffers_precondition (void)
{
  return !buffer_queue_empty (&destroy_queue);
}

BEGIN_INTERNAL (NO_PARAMETER, DESTROY_BUFFERS_NO, "", "", destroy_buffers, int param)
{
  {
    const char* s = "registry: destroy_buffers\n";
    syslog (s, strlen (s));
  }

  initialize ();
  scheduler_remove (DESTROY_BUFFERS_NO, param);
  if (destroy_buffers_precondition ()) {
    /* Drain the queue. */
    while (!buffer_queue_empty (&destroy_queue)) {
      buffer_destroy (buffer_queue_item_bd (buffer_queue_front (&destroy_queue)));
      buffer_queue_pop (&destroy_queue);
    }
  }
  schedule ();
  scheduler_finish (false, -1);
}

static void
schedule (void)
{
  if (!buffer_queue_empty (&response_queue)) {
    scheduler_add (TMPFS_RESPONSE_NO, buffer_queue_item_parameter (buffer_queue_front (&response_queue)));
  }
  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }
}
