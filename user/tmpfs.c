#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <dymem.h>
#include <buffer_file.h>
#include "cpio.h"

/*
  Tmpfs
  =====
  Tmpfs is an in-memory file system based on the same principles as the tmpfs system of Linux.
*/

/* Every node in the filesystem is either a file or directory. */
typedef enum {
  FILE,
  DIRECTORY,
} node_type_t;

typedef struct node_struct node_t;
struct node_struct {
  node_type_t type;
  size_t name_size;
  char* name;
  bd_t bd;
  size_t bd_size;
  size_t size;
  size_t parent;
  size_t first_child;
  size_t next;
};

/* All of the nodes are stored in a dynamic array. */
static size_t nodes_capacity = 0;
static size_t nodes_size = 0;
static node_t* nodes = 0;
static size_t root;

static void
reserve (size_t capacity)
{
  if (nodes_capacity < capacity) {
    nodes_capacity = capacity * 2;
    nodes = realloc (nodes, nodes_capacity * sizeof (node_t));
  }
}

static size_t
node_create (node_type_t type,
	     const char* name,
	     size_t name_size,
	     bd_t bd,
	     size_t bd_size,
	     size_t size,
	     size_t parent)
{
  reserve (nodes_size + 1);

  /* Acquire an index for this node. */
  size_t idx = nodes_size++;

  nodes[idx].type = type;
  nodes[idx].name_size = name_size;
  nodes[idx].name = malloc (name_size);
  memcpy (nodes[idx].name, name, name_size);
  nodes[idx].bd = bd;
  nodes[idx].size = size;
  nodes[idx].parent = parent;
  nodes[idx].first_child = -1;
  nodes[idx].next = -1;

  return idx;
}

static size_t
directory_find_or_create (size_t parent,
			  const char* name,
			  size_t name_size)
{
  /* Reserve space so the call to node_create won't invalidate the idx pointer. */
  reserve (nodes_size + 1);  

  /* Iterate over the parent's children looking for the name. */
  size_t* idx;
  for (idx = &nodes[parent].first_child; *idx != -1; idx = &nodes[*idx].next) {
    if (nodes[*idx].name_size == name_size &&
  	memcmp (nodes[*idx].name, name, name_size) == 0) {
      return *idx;
    }
  }

  /* Create a node. */
  size_t n = node_create (DIRECTORY, name, name_size, -1, 0, 0, parent);
  *idx = n;

  return n;
}

static size_t
file_create_or_replace (size_t parent,
			const char* name,
			size_t name_size,
			bd_t bd,
			size_t bd_size,
			size_t size)
{
  /* Reserve space so the call to node_create won't invalidate the idx pointer. */
  reserve (nodes_size + 1);  

  /* Iterate over the parent's children looking for the name. */
  size_t* idx;
  for (idx = &nodes[parent].first_child; *idx != -1; idx = &nodes[*idx].next) {
    if (nodes[*idx].name_size == name_size &&
  	memcmp (nodes[*idx].name, name, name_size) == 0) {
      return *idx;
    }
  }

  /* Create a node. */
  size_t n = node_create (FILE, name, name_size, buffer_copy (bd, 0, bd_size), bd_size, size, parent);
  *idx = n;

  return n;
}

static void
print (size_t idx)
{
  if (idx != -1) {
    /* Print this node. */
    switch (nodes[idx].type) {
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
    syslog (nodes[idx].name, nodes[idx].name_size);
    syslog ("\n", 1);

    /* Print the children. */
    for (size_t c = nodes[idx].first_child; c != -1; c = nodes[c].next) {
      print (c);
    }
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
  /* Allocate an array of nodes. */
  nodes_capacity = 1;
  nodes_size = 0;
  nodes = malloc (nodes_capacity * sizeof (node_t));

  /* Create the root. */
  root = node_create (DIRECTORY, "", 0, -1, 0, 0, -1);
  /* TODO:  Do we need to make the root its own parent? */

  if (ptr != 0) {
    /* Parse the cpio archive looking for files that we need. */
    buffer_file_t bf;
    buffer_file_open (&bf, bd, bd_size, ptr, false);
    
    cpio_file_t* file;
    while ((file = parse_cpio (&bf)) != 0) {

      /* Ignore the "." directory. */
      if (strcmp (file->name, ".") != 0) {

      	size_t parent = root;

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
	  file_create_or_replace (parent, begin, file->name_size - 1 - (begin - file->name), file->bd, file->bd_size, file->size);
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
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, AUTO_MAP, INIT, init);

static void
schedule (void)
{
  /* TODO */
}
