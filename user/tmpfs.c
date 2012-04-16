#include <automaton.h>
#include <string.h>
#include <dymem.h>
#include <buffer_file.h>
#include "cpio.h"
#include "vfs_msg.h"
#include "syslog.h"
#include <description.h>

/*
  Tmpfs
  =====
  Tmpfs is an in-memory file system based on the tmpfs system of Linux.
  Tmpfs is initialized with a cpio archive that contains the initial contents of the file system.
  Currently, only files and directories are supported.

  Tmpfs is designed to work with a single virtual file system.
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

  I removed auto-mapping so that part of the discussion is no longer valid.
  However, the control message vs. bulk data argument remains.
  What we really want is a way to send two buffers at once:  one for the control message and one for the data.
  Since this pattern seems generally applicable, I decided to change the kernel to pass two buffers instead of one when executing a output/input binding.
  Thus, tmpfs and all other automata that move bulk data without processing it can be simplified.
  
  Implementation Details
  ----------------------
  The main data structure is the inode.
  Each inode has an id, a type, and a name.
  File inodes have a buffer.
  All inodes have a link to their parent, the first child, and their next sibling.
  This allows them to be organized into a tree structure.
  The top-level inode is called the root.

  The inode list is a vector that allows one to lookup an inode given its id.
  Thus, the first part of most requests will involve a table look-up to translate the given id into an inode.

  The response queue is a queue of buffers that contain responses.
  Currently, requests are processed immediately and the response placed on the queue.
  An alternate strategy might use a request queue to buffer requests and an internal action that converts requests to responses.
  
  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define INIT_NO 1
#define STOP_NO 2
#define SYSLOG_NO 3
#define TMPFS_REQUEST_NO 4
#define TMPFS_RESPONSE_NO 5

/* Every inode in the filesystem is either a file or directory. */
typedef struct inode inode_t;
struct inode {
  vfs_fs_node_t node;
  size_t name_size;
  char* name; /* Does not contain a terminating 0. */
  bd_t bd;
  size_t size;
  inode_t* parent;
  inode_t* first_child;
  inode_t* next_sibling;
};

/* A vector that maps id to inode. */
static size_t nodes_capacity = 0;
static size_t nodes_size = 0;
static inode_t** nodes = 0;

/* A list of free inodes. */
static inode_t* free_list = 0;

/* Initialization flag. */
static bool initialized = false;

#define ERROR "tmpfs: error: "

typedef enum {
  RUN,
  STOP,
} state_t;
static state_t state = RUN;

/* Output buffer for syslog. */
static bd_t syslog_bd = -1;
static buffer_file_t syslog_buffer;

/* Queue of response. */
static bd_t response_bda = -1;
static bd_t response_bdb = -1;
static vfs_fs_response_queue_t response_queue;

static inode_t*
inode_create (vfs_fs_node_type_t type,
	     const char* name,
	     size_t name_size,
	     bd_t bd,
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
    node->node.id = nodes_size++;
  }

  /* Place the node in the index. */
  nodes[node->node.id] = node;

  /* Initialize the node. */
  node->node.type = type;
  node->name_size = name_size;
  node->name = malloc (name_size);
  memcpy (node->name, name, name_size);
  node->bd = bd;
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
  *ptr = inode_create (DIRECTORY, name, name_size, -1, 0, parent);

  return *ptr;
}

static inode_t*
file_find_or_create (inode_t* parent,
		     const char* name,
		     size_t name_size,
		     bd_t bd,
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
  *ptr = inode_create (FILE, name, name_size, buffer_copy (bd, 0), size, parent);

  return *ptr;
}

/* static void */
/* print (inode_t* node) */
/* { */
/*   if (node != 0) { */
/*     /\* Print this node. *\/ */
/*     switch (node->node.type) { */
/*     case FILE: */
/*       bfprintf (&syslog_buffer, "FILE "); */
/*       break; */
/*     case DIRECTORY: */
/*       bfprintf (&syslog_buffer, "DIRECTORY "); */
/*       break; */
/*     } */
/*     buffer_file_write (&syslog_buffer, node->name, node->name_size); */
/*     buffer_file_put (&syslog_buffer, '\n'); */

/*     /\* Print the children. *\/ */
/*     for (inode_t* child = node->first_child; child != 0; child = child->next_sibling) { */
/*       print (child); */
/*     } */
/*   } */
/* } */

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    /* Create the syslog buffer. */
    syslog_bd = buffer_create (0, 0);
    if (syslog_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&syslog_buffer, syslog_bd) != 0) {
      exit ();
    }

    aid_t syslog_aid = lookup (SYSLOG_NAME, strlen (SYSLOG_NAME) + 1, 0);
    if (syslog_aid != -1) {
      /* Bind to the syslog. */

      description_t syslog_description;
      if (description_init (&syslog_description, syslog_aid) != 0) {
	exit ();
      }
      
      action_desc_t syslog_text_in;
      if (description_read_name (&syslog_description, &syslog_text_in, SYSLOG_TEXT_IN) != 0) {
	exit ();
      }
            
      /* We bind the response first so they don't get lost. */
      if (bind (getaid (), SYSLOG_NO, 0, syslog_aid, syslog_text_in.number, 0, 0) == -1) {
	exit ();
      }

      description_fini (&syslog_description);
    }

    /* Allocate the inode vector. */
    nodes_capacity = 1;
    nodes_size = 0;
    nodes = malloc (nodes_capacity * sizeof (inode_t*));
    
    /* Create the root. */
    nodes[nodes_size++] = inode_create (DIRECTORY, "", 0, -1, 0, 0);
    
    /* TODO:  Do we need to make the root its own parent? */

    response_bda = buffer_create (0, 0);
    response_bdb = buffer_create (0, 0);
    if (response_bda == -1 ||
	response_bdb == -1) {
      bfprintf (&syslog_buffer, ERROR "could not create response buffer\n");
      state = STOP;
      return;
    }
    vfs_fs_response_queue_init (&response_queue);

    bd_t bda = getinita ();
    bd_t bdb = getinitb ();

    /* Parse the cpio archive looking for files that we need. */
    cpio_archive_t archive;
    if (cpio_archive_init (&archive, bda) == 0) {
      
      cpio_file_t* file;
      while ((file = cpio_archive_next_file (&archive)) != 0) {
	
	/* Ignore the "." directory. */
	if (strcmp (file->name, ".") != 0) {
	  
	  /* Start at the root. */
	  inode_t* parent = nodes[0];
	  
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
	    file_find_or_create (parent, begin, file->name_size - 1 - (begin - file->name), file->bd, file->size);
	    break;
	  case CPIO_DIRECTORY:
	    directory_find_or_create (parent, begin, file->name_size - 1 - (begin - file->name));
	    break;
	  }
	}
	
	/* Destroy the cpio file. */
	cpio_file_destroy (file);
      }
    }

    if (bda != -1) {
      buffer_destroy (bda, 0);
    }
    if (bdb != -1) {
      buffer_destroy (bdb, 0);
    }
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

BEGIN_INPUT (NO_PARAMETER, TMPFS_REQUEST_NO, VFS_FS_REQUEST_NAME, "", request, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  vfs_fs_type_t type;
  if (read_vfs_fs_request_type (bda, bdb, &type) != 0) {
    vfs_fs_response_queue_push_bad_request (&response_queue, type);
    finish_input (bda, bdb);
  }
  
  switch (type) {
  case VFS_FS_DESCEND:
    {
      size_t id;
      const char* name;
      size_t name_size;
      vfs_fs_node_t no_node;
      if (read_vfs_fs_descend_request (bda, bdb, &id, &name, &name_size) != 0) {
  	vfs_fs_response_queue_push_descend (&response_queue, VFS_FS_BAD_REQUEST, &no_node);
  	finish_input (bda, bdb);
      }

      if (id >= nodes_size) {
  	vfs_fs_response_queue_push_descend (&response_queue, VFS_FS_BAD_NODE, &no_node);
  	finish_input (bda, bdb);
      }

      inode_t* inode = nodes[id];

      if (inode == 0) {
  	vfs_fs_response_queue_push_descend (&response_queue, VFS_FS_BAD_NODE, &no_node);
  	finish_input (bda, bdb);
      }

      if (inode->node.type != DIRECTORY) {
  	vfs_fs_response_queue_push_descend (&response_queue, VFS_FS_NOT_DIRECTORY, &no_node);
  	finish_input (bda, bdb);
      }

      inode_t* child;
      for (child = inode->first_child; child != 0; child = child->next_sibling) {
  	if (child->name_size == name_size &&
  	    memcmp (child->name, name, name_size) == 0) {
  	  /* Found the child with the correct name. */
  	  vfs_fs_response_queue_push_descend (&response_queue, VFS_FS_SUCCESS, &child->node);
  	  finish_input (bda, bdb);
  	}
      }

      /* Didn't find it. */
      vfs_fs_response_queue_push_descend (&response_queue, VFS_FS_CHILD_DNE, &no_node);
      finish_input (bda, bdb);
    }
    break;
  case VFS_FS_READFILE:
    {
      size_t id;
      if (read_vfs_fs_readfile_request (bda, bdb, &id) != 0) {
  	vfs_fs_response_queue_push_readfile (&response_queue, VFS_FS_BAD_REQUEST, 0, -1);
  	finish_input (bda, bdb);
      }

      if (id >= nodes_size) {
  	vfs_fs_response_queue_push_readfile (&response_queue, VFS_FS_BAD_NODE, 0, -1);
  	finish_input (bda, bdb);
      }

      inode_t* inode = nodes[id];

      if (inode == 0) {
  	vfs_fs_response_queue_push_readfile (&response_queue, VFS_FS_BAD_NODE, 0, -1);
  	finish_input (bda, bdb);
      }

      if (inode->node.type != FILE) {
  	vfs_fs_response_queue_push_readfile (&response_queue, VFS_FS_NOT_FILE, 0, -1);
  	finish_input (bda, bdb);
      }

      if (vfs_fs_response_queue_push_readfile (&response_queue, VFS_FS_SUCCESS, inode->size, inode->bd) != 0) {
	bfprintf (&syslog_buffer, ERROR "could not enqueue readfile response\n");
	state = STOP;
	finish_input (bda, bdb);
      }
      finish_input (bda, bdb);
    }
    break;
  default:
    vfs_fs_response_queue_push_bad_request (&response_queue, VFS_UNKNOWN);
    finish_input (bda, bdb);
    break;
  }

  finish_input (bda, bdb);
}

static bool
response_precondition (void)
{
  return !vfs_fs_response_queue_empty (&response_queue);
}

BEGIN_OUTPUT (NO_PARAMETER, TMPFS_RESPONSE_NO, VFS_FS_RESPONSE_NAME, "", response, ano_t ano, int param)
{
  initialize ();

  if (response_precondition ()) {
    if (vfs_fs_response_queue_pop_to_buffer (&response_queue, response_bda, response_bdb) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not enqueue response\n");
      state = STOP;
      finish_output (false, -1, -1);
    }
    finish_output (true, response_bda, response_bdb);
  }
  else {
    finish_output (false, -1, -1);
  }
}

void
do_schedule (void)
{
  if (stop_precondition ()) {
    schedule (STOP_NO, 0, 0);
  }
  if (syslog_precondition ()) {
    schedule (SYSLOG_NO, 0, 0);
  }
  if (response_precondition ()) {
    schedule (TMPFS_RESPONSE_NO, 0, 0);
  }
}
