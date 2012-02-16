#include "tmpfs.h"
#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <dymem.h>
#include <buffer_file.h>
#include <buffer_queue.h>
#include "cpio.h"

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

  The primary interface of tmpfs involves a control channel and a data channel.
  The control channel contains a input action for receive requests and an output action for indicating the results of a request.
  The data channel contains a input for receiving bulk data, e.g., the user is performing a write, and an output for sending bulk data, e.g., the user is performing a read.
  Separating the channels reduces the amount of buffer manipulation and mapping.
  The control channel will usually be auto-mapped since the data traversing the channel is small.
  The data channel will usually only be auto-mapped on the user's input channel as that is the only time the data is actually inspected.
  Combining the control and data also forces one to prepend a header on every data buffer which is tedious.

  Supported Operations
  --------------------

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

#define TMPFS_CONTROL_REQUEST_NO 1
#define TMPFS_CONTROL_REQUEST_NAME "control_request"
#define TMPFS_CONTROL_REQUEST_DESC ""

#define TMPFS_CONTROL_RESPONSE_NO 2
#define TMPFS_CONTROL_RESPONSE_NAME "control_response"
#define TMPFS_CONTROL_RESPONSE_DESC ""

#define TMPFS_DATA_WRITE_NO 3
#define TMPFS_DATA_WRITE_NAME "data_write"
#define TMPFS_DATA_WRITE_DESC ""

#define TMPFS_DATA_READ_NO 4
#define TMPFS_DATA_READ_NAME "data_read"
#define TMPFS_DATA_READ_DESC ""

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

/* Queue for control responses. */
static buffer_queue_t control_response_queue;

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

    buffer_queue_init (&control_response_queue);
  }
}

static void
form_control_response (aid_t aid,
		       tmpfs_error_t error)
{
  /* Create a response. */
  bd_t bd = buffer_create (sizeof (tmpfs_control_response_t));
  tmpfs_control_response_t* rr = buffer_map (bd);
  rr->error = error;
  /* We don't unmap it because we are going to destroy it when we respond. */

  buffer_queue_push (&control_response_queue, aid, bd);
}

static void
process_control_request (aid_t aid,
			 bd_t bd,
			 size_t bd_size,
			 void* ptr)
{
  if (bd == -1) {
    form_control_response (aid, TMPFS_NO_BUFFER);
    return;
  }

  if (ptr == 0) {
    form_control_response (aid, TMPFS_BUFFER_TOO_BIG);
    return;
  }

  buffer_file_t file;
  buffer_file_open (&file, bd, bd_size, ptr, false);

  const tmpfs_control_request_t* r = buffer_file_readp (&file, sizeof (tmpfs_control_request_t));
  if (r == 0) {
    form_control_response (aid, TMPFS_BAD_REQUEST);
    return;
  }

  switch (r->control) {
  case TMPFS_READ_FILE:
    {
      if (r->u.read_file.inode >= nodes_size) {
	form_control_response (aid, TMPFS_BAD_INODE);
	return;
      }

      inode_t* inode = nodes[r->u.read_file.inode];

      if (inode == 0) {
	form_control_response (aid, TMPFS_BAD_INODE);
	return;
      }

      /* TODO:  Put the buffer on the data queue. */

      form_control_response (aid, TMPFS_SUCCESS);
      return;
    }
    break;
  default:
    form_control_response (aid, TMPFS_BAD_REQUEST);
    return;
  }
  
}

static void
schedule (void);

void
init (int param,
      bd_t bd,
      size_t bd_size,
      void* ptr)
{
  initialize ();

  if (ptr != 0) {
    /* Parse the cpio archive looking for files that we need. */
    buffer_file_t bf;
    buffer_file_open (&bf, bd, bd_size, ptr, false);
    
    cpio_file_t* file;
    while ((file = parse_cpio (&bf)) != 0) {

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

    print (root);

    const char* s = "tmpfs: starting\n";
    syslog (s, strlen (s));
  }

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, PARAMETER, AUTO_MAP, init, INIT, "", "");


void
control_request (aid_t aid,
		 bd_t bd,
		 size_t bd_size,
		 void* ptr)
{
  initialize ();

  process_control_request (aid, bd, bd_size, ptr);

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, AUTO_MAP, control_request, TMPFS_CONTROL_REQUEST_NO, TMPFS_CONTROL_REQUEST_NAME, TMPFS_CONTROL_REQUEST_DESC);

void
control_response (aid_t aid,
		  size_t bc)
{
  initialize ();

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&control_response_queue, aid);

  if (item != 0) {
    /* Found a response.  Execute. */
    bd_t bd = buffer_queue_item_bd (item);
    buffer_queue_erase (&control_response_queue, item);

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    /* Did not find a response. */
    schedule ();
    scheduler_finish (-1, FINISH_NOOP);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, AUTO_PARAMETER, 0, control_response, TMPFS_CONTROL_RESPONSE_NO, TMPFS_CONTROL_RESPONSE_NAME, TMPFS_CONTROL_RESPONSE_DESC);

void
data_write (aid_t aid,
	    bd_t bd,
	    size_t bd_size,
	    void* ptr)
{
  initialize ();

  /* TODO */
  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, 0, data_write, TMPFS_DATA_WRITE_NO, TMPFS_DATA_WRITE_NAME, TMPFS_DATA_WRITE_DESC);

void
data_read (aid_t aid,
	       size_t bc)
{
  initialize ();
  
  /* TODO */
  schedule ();
  scheduler_finish (-1, FINISH_NOOP);
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, AUTO_PARAMETER, 0, data_read, TMPFS_DATA_READ_NO, TMPFS_DATA_READ_NAME, TMPFS_DATA_READ_DESC);

static void
schedule (void)
{
  if (!buffer_queue_empty (&control_response_queue)) {
    scheduler_add (TMPFS_CONTROL_RESPONSE_NO, buffer_queue_item_parameter (buffer_queue_front (&control_response_queue)));
  }
  /* TODO */
}
