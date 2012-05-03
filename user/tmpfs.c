#include <automaton.h>
#include <string.h>
#include <dymem.h>
#include <buffer_file.h>
#include "cpio.h"
#include "fs_msg.h"

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

#define REQUEST_NO 1
#define RESPONSE_NO 2
#define FS_DESCEND_REQUEST_IN_NO 3
#define FS_DESCEND_RESPONSE_OUT_NO 4
#define FS_READFILE_REQUEST_IN_NO 5
#define FS_READFILE_RESPONSE_OUT_NO 6

/* Every inode in the filesystem is either a file or directory. */
typedef struct inode inode_t;
struct inode {
  fs_nodeid_t nodeid;
  fs_node_type_t type;
  char* name; /* Not 0 terminated. */
  size_t name_size;
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

static bd_t output_bda = -1;
static buffer_file_t output_bfa;

/* /\* Queue of response. *\/ */
/* static bd_t response_bda = -1; */
/* static bd_t response_bdb = -1; */
/* static vfs_fs_response_queue_t response_queue; */

#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "
#define INFO __FILE__ ": info: "

/* static void */
/* print (const inode_t* inode) */
/* { */
/*   snprintf (log_buffer, LOG_BUFFER_SIZE, "nodeid=%d\n", inode->nodeid); */
/*   logs (log_buffer); */
/*   for (const inode_t* c = inode->first_child; c != 0; c = c->next_sibling) { */
/*     print (c); */
/*   } */
/* } */

static inode_t*
inode_create (fs_node_type_t type,
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
      nodes_capacity = nodes_capacity * 2 + 1;
      nodes = realloc (nodes, nodes_capacity * sizeof (inode_t*));
      if (nodes == 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "out of memory: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }
    }

    /* Create a new node. */
    node = malloc (sizeof (inode_t));
    if (node == NULL) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "out of memory: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    node->nodeid = nodes_size++;
  }

  /* Place the node in the index. */
  nodes[node->nodeid] = node;

  /* Initialize the node. */
  node->type = type;
  node->name = malloc (name_size);
  memcpy (node->name, name, name_size);
  node->name_size = name_size;
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
  *ptr = inode_create (FILE, name, name_size, bd, size, parent);

  return *ptr;
}

typedef struct client client_t;
struct client {
  aid_t aid;
  bd_t response_bda;
  bd_t response_bdb;
  buffer_file_t response_buffer;
  client_t* next;
};

static client_t* client_head = 0;

static client_t*
find_client (aid_t aid)
{
  client_t* client;
  for (client = client_head; client != 0; client = client->next) {
    if (client->aid == aid) {
      break;
    }
  }

  return client;
}

static client_t*
create_client (aid_t aid)
{
  client_t* client = malloc (sizeof (client_t));
  memset (client, 0, sizeof (client_t));
  client->aid = aid;
  client->response_bda = buffer_create (0);
  client->response_bdb = buffer_create (0);
  buffer_file_initw (&client->response_buffer, client->response_bda);
  client->next = client_head;
  client_head = client;
  return client;
}

typedef struct descend_response descend_response_t;
struct descend_response {
  aid_t to;
  fs_descend_response_t response;
  descend_response_t* next;
};

static descend_response_t* descend_response_head = 0;
static descend_response_t** descend_response_tail = &descend_response_head;

static void
push_descend_response (aid_t to,
		       fs_descend_error_t error,
		       fs_nodeid_t nodeid)
{
  descend_response_t* res = malloc (sizeof (descend_response_t));
  memset (res, 0, sizeof (descend_response_t));
  res->to = to;
  fs_descend_response_init (&res->response, error, nodeid);

  *descend_response_tail = res;
  descend_response_tail = &res->next;
}

static void
pop_descend_response (void)
{
  descend_response_t* res = descend_response_head;
  descend_response_head = res->next;
  if (descend_response_head == 0) {
    descend_response_tail = &descend_response_head;
  }
  free (res);
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    output_bda = buffer_create (0);
    if (output_bda == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create output buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (buffer_file_initw (&output_bfa, output_bda) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize output buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    /* Allocate the inode vector. */
    nodes_capacity = 1;
    nodes_size = 0;
    nodes = malloc (nodes_capacity * sizeof (inode_t*));
    if (nodes == NULL) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "out of memory: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    
    /* Create the root. */
    inode_create (DIRECTORY, 0, 0, -1, 0, 0);
    
    /* TODO:  Do we need to make the root its own parent? */

    bd_t bda = getinita ();
    bd_t bdb = getinitb ();

    /* Parse the cpio archive looking for files that we need. */
    cpio_archive_t archive;
    if (cpio_archive_init (&archive, bda) == 0) {
      cpio_file_t file;
      while (cpio_archive_read (&archive, &file) == 0) {
	/* Ignore the "." directory. */
	if (strcmp (file.name, ".") != 0) {
	  
	  /* Start at the root. */
	  inode_t* parent = nodes[0];
	  
	  const char* begin;
	  for (begin = file.name; begin < file.name + file.name_size;) {
	    /* Find the delimiter. */
	    const char* end = memchr (begin, '/', file.name_size - (begin - file.name));
	    
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
	  
	  switch (file.mode & CPIO_TYPE_MASK) {
	  case CPIO_REGULAR:
	    file_find_or_create (parent, begin, file.name_size - 1 - (begin - file.name), cpio_file_read (&archive, &file), file.file_size);
	    break;
	  case CPIO_DIRECTORY:
	    directory_find_or_create (parent, begin, file.name_size - 1 - (begin - file.name));
	    break;
	  }
	}
      }
    }

    /* print (nodes[0]); */

    if (bda != -1) {
      buffer_destroy (bda);
    }
    if (bdb != -1) {
      buffer_destroy (bdb);
    }
  }
}

BEGIN_INPUT (AUTO_PARAMETER, REQUEST_NO, FS_REQUEST_NAME, "", request, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  client_t* client = find_client (aid);
  if (client == 0) {
    client = create_client (aid);
  }

  buffer_file_t buffer;
  if (buffer_file_initr (&buffer, bda) != 0) {
    if (fs_descend_response_write (&client->response_buffer, FS_BAD_REQUEST, -1) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write response: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    finish_input (bda, bdb);
  }

  fs_type_t type;
  if (fs_request_type_read (&buffer, &type) != 0) {
    if (fs_descend_response_write (&client->response_buffer, FS_BAD_REQUEST, -1) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write response: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    finish_input (bda, bdb);
  }

  switch (type) {
  case FS_DESCEND:
    break;
  case FS_READFILE:
    {
      fs_nodeid_t nodeid;
      if (fs_readfile_request_read (&buffer, &nodeid) != 0) {
	if (fs_readfile_response_write (&client->response_buffer, FS_BAD_REQUEST, -1) != 0) {
	  snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write response: %s", lily_error_string (lily_error));
	  logs (log_buffer);
	  exit (-1);
	}
	finish_input (bda, bdb);
      }
      
      if (nodeid >= nodes_size) {
	if (fs_readfile_response_write (&client->response_buffer, FS_NODE_DNE, -1) != 0) {
	  snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write response: %s", lily_error_string (lily_error));
	  logs (log_buffer);
	  exit (-1);
	}
	finish_input (bda, bdb);
      }
      
      inode_t* inode = nodes[nodeid];
      
      if (inode == 0) {
	if (fs_readfile_response_write (&client->response_buffer, FS_NODE_DNE, -1) != 0) {
	  snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write response: %s", lily_error_string (lily_error));
	  logs (log_buffer);
	  exit (-1);
	}
	finish_input (bda, bdb);
      }
      
      if (inode->type != FILE) {
	if (fs_readfile_response_write (&client->response_buffer, FS_NOT_FILE, -1) != 0) {
	  snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write response: %s", lily_error_string (lily_error));
	  logs (log_buffer);
	  exit (-1);
	}
	finish_input (bda, bdb);
      }

      if (buffer_assign (client->response_bdb, inode->bd, 0, buffer_size (inode->bd)) != 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not assign buffer: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }

      if (fs_readfile_response_write (&client->response_buffer, FS_SUCCESS, inode->size) != 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write response: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }
      finish_input (bda, bdb);
    }
    break;
  }

  /* The response type doesn't matter. */
  if (fs_readfile_response_write (&client->response_buffer, FS_BAD_REQUEST, -1) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write response: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }

  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, RESPONSE_NO, FS_RESPONSE_NAME, "", response, ano_t ano, aid_t aid)
{
  initialize ();

  client_t* client = find_client (aid);
  if (client != 0 && buffer_file_size (&client->response_buffer) != 0) {
    buffer_file_truncate (&client->response_buffer);
    finish_output (true, client->response_bda, client->response_bdb);
  }

  finish_output (false, -1, -1);
}

BEGIN_INPUT (AUTO_PARAMETER, FS_DESCEND_REQUEST_IN_NO, FS_DESCEND_REQUEST_IN_NAME, "", fs_descend_request_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t bf;
  if (buffer_file_initr (&bf, bda) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not initialize descend request buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  fs_descend_request_t* req = fs_descend_request_read (&bf);
  if (req == 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not read descend request: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  if (req->nodeid >= nodes_size) {
    push_descend_response (aid, FS_DESCEND_NODE_DNE, -1);
    fs_descend_request_destroy (req);
    finish_input (bda, bdb);
  }

  inode_t* inode = nodes[req->nodeid];
  
  if (inode == 0) {
    push_descend_response (aid, FS_DESCEND_NODE_DNE, -1);
    fs_descend_request_destroy (req);
    finish_input (bda, bdb);
  }

  if (inode->type != DIRECTORY) {
    push_descend_response (aid, FS_DESCEND_NOT_DIRECTORY, -1);
    fs_descend_request_destroy (req);
    finish_input (bda, bdb);
  }

  for (inode_t* child = inode->first_child; child != 0; child = child->next_sibling) {
    if (pstrcmp (child->name, child->name + child->name_size, req->name_begin, req->name_end) == 0) {
      /* Found the child with the correct name. */
      push_descend_response (aid, FS_DESCEND_SUCCESS, child->nodeid);
      fs_descend_request_destroy (req);
      finish_input (bda, bdb);
    }
  }

  /* Didn't find it. */
  push_descend_response (aid, FS_DESCEND_CHILD_DNE, -1);
  fs_descend_request_destroy (req);
  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, FS_DESCEND_RESPONSE_OUT_NO, FS_DESCEND_RESPONSE_OUT_NAME, "", fs_descend_response_out, ano_t ano, aid_t aid)
{
  initialize ();
  
  if (descend_response_head != 0 && descend_response_head->to == aid) {
    buffer_file_shred (&output_bfa);
    buffer_file_write (&output_bfa, &descend_response_head->response, sizeof (fs_descend_response_t));
    pop_descend_response ();
    finish_output (true, output_bfa.bd, -1);
  }
  finish_output (false, -1, -1);
}

BEGIN_INPUT (AUTO_PARAMETER, FS_READFILE_REQUEST_IN_NO, FS_READFILE_REQUEST_IN_NAME, "", fs_readfile_request_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  logs (__func__);
  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, FS_READFILE_RESPONSE_OUT_NO, FS_READFILE_RESPONSE_OUT_NAME, "", fs_readfile_response_out, ano_t ano, aid_t aid)
{
  initialize ();
  logs (__func__);
  finish_output (false, -1, -1);
}

void
do_schedule (void)
{
  if (descend_response_head != 0) {
    schedule (FS_DESCEND_RESPONSE_OUT_NO, descend_response_head->to);
  }
  for (client_t* client = client_head; client != 0; client = client->next) {
    if (buffer_file_size (&client->response_buffer) != 0) {
      if (schedule (RESPONSE_NO, client->aid) != 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not schedule response action: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }
    }
  }
}
