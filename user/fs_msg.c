#include "fs_msg.h"
#include <dymem.h>
#include <string.h>
#include <automaton.h>
#include <description.h>

typedef void (*descend_callback_t) (void* arg, fs_error_t error, fs_nodeid_t nodeid);

typedef struct fs_op fs_op_t;
struct fs_op {
  fs_type_t type;
  union {
    struct {
      fs_nodeid_t nodeid;
      const char* begin;
      const char* end;
      descend_callback_t callback;
      void* arg;
    } descend;
    struct {
      fs_nodeid_t nodeid;
      readfile_callback_t callback;
      void* arg;
    } readfile;
  } u;
  fs_op_t* next;
};

struct fs {
  size_t refcount;
  aid_t aid;
  bool bound;
  bd_t request_bda;
  bd_t request_bdb;
  buffer_file_t request_buffer;
  bool sent;
  fs_op_t* op_head;
  fs_op_t** op_tail;
  fs_t* next;
};

/* A vnode represents an entry in a virtual file system. */
typedef struct {
  aid_t aid;
  fs_nodeid_t nodeid;
} vnode_t;

struct redirect {
  vnode_t from;
  vnode_t to;
  redirect_t* next;
};

static bool
vnode_equal (const vnode_t* x,
	     const vnode_t* y)
{
  return x->aid == y->aid && x->nodeid == y->nodeid;
}

static fs_t*
find_fs (const vfs_t* vfs,
	 aid_t aid)
{
  fs_t* fs;
  for (fs = vfs->fs_head; fs != 0; fs = fs->next) {
    if (fs->aid == aid) {
      break;
    }
  }

  return fs;
}

static fs_t*
create_fs (vfs_t* vfs,
	   aid_t aid)
{
  /* Create the file system. */
  fs_t* fs = malloc (sizeof (fs_t));
  memset (fs, 0, sizeof (fs_t));
  fs->refcount = 0;
  fs->aid = aid;
  fs->bound = true;
  fs->op_tail = &fs->op_head;
  fs->request_bda = buffer_create (0);
  fs->request_bdb = buffer_create (0);
  buffer_file_initw (&fs->request_buffer, fs->request_bda);
  fs->sent = false;

  /* Insert into the list. */
  fs->next = vfs->fs_head;
  vfs->fs_head = fs;

  /* Bind. */
  description_t description;
  if (description_init (&description, aid) != 0) {
    /* Could not describe. */
    fs->bound = false;
  }

  action_desc_t request;
  if (description_read_name (&description, &request, FS_REQUEST_NAME) != 0) {
    fs->bound = false;
  }
  
  action_desc_t response;
  if (description_read_name (&description, &response, FS_RESPONSE_NAME) != 0) {
    fs->bound = false;
  }
  
  aid_t this_aid = getaid ();
  if (bind (aid, response.number, 0, this_aid, vfs->response, 0) == -1 ||
      bind (this_aid, vfs->request, 0, aid, request.number, 0) == -1) {
    fs->bound = false;
  }
  
  description_fini (&description);

  return fs;
}

static void
fs_incref (fs_t* fs)
{
  ++fs->refcount;
}

static void
fs_decref (fs_t* fs)
{
  /* TODO */
  --fs->refcount;
}

static void
fs_op_push (fs_t* fs,
	    fs_op_t* op)
{
  *fs->op_tail = op;
  fs->op_tail = &op->next;
  fs_incref (fs);
}

static void
fs_op_pop (fs_t* fs)
{
  fs_op_t* op = fs->op_head;
  fs->op_head = op->next;
  if (fs->op_head == 0) {
    fs->op_tail = &fs->op_head;
  }
  free (op);
  fs_decref (fs);
}

static bool
fs_op_empty (const fs_t* fs)
{
  return fs->op_head == 0;
}

static bool
fs_request_precondition (const fs_t* fs)
{
  return !fs->sent && !fs_op_empty (fs);
}

static void
fs_request (fs_t* fs)
{
  if (fs_request_precondition (fs)) {
    buffer_file_truncate (&fs->request_buffer);
    /* TODO */
    buffer_resize (fs->request_bdb, 0);

    fs_op_t* op = fs->op_head;
    switch (op->type) {
    case FS_DESCEND:
      /* TODO */
      fs_descend_request_write (&fs->request_buffer, op->u.descend.nodeid, op->u.descend.begin, op->u.descend.end - op->u.descend.begin);
      break;
    case FS_READFILE:
      /* TODO */
      fs_readfile_request_write (&fs->request_buffer, op->u.readfile.nodeid);
      break;
    }

    fs->sent = true;
    finish_output (true, fs->request_bda, fs->request_bdb);
  }
  else {
    finish_output (false, -1, -1);
  }
}

static void
fs_response (fs_t* fs,
	     bd_t bda,
	     bd_t bdb)
{
  if (!fs->sent) {
    /* Response when none was expected. */
    finish_input (bda, bdb);
  }

  buffer_file_t buffer;
  if (buffer_file_initr (&buffer, bda) != 0) {
    /* TODO */
    finish_input (bda, bdb);
  }    

  fs_op_t* op = fs->op_head;
  switch (op->type) {
  case FS_DESCEND:
    {
      fs_error_t error;
      fs_nodeid_t nodeid;
      if (fs_descend_response_read (&buffer, &error, &nodeid) == 0) {
	op->u.descend.callback (op->u.descend.arg, error, nodeid);
      }
      else {
	/* TODO */
      }
    }
    break;
  case FS_READFILE:
  {
      fs_error_t error;
      size_t size;
      if (fs_readfile_response_read (&buffer, &error, &size) == 0 &&
	  buffer_size (bdb) >= size_to_pages (size)) {
	op->u.readfile.callback (op->u.readfile.arg, error, size, bdb);
      }
      else {
	/* TODO */
      }
    }
      break;
  }
  
  fs_op_pop (fs);
  fs->sent = false;

  finish_input (bda, bdb);
}

static void
fs_schedule (const vfs_t* vfs,
	     const fs_t* fs)
{
  if (fs_request_precondition (fs)) {
    schedule (vfs->request, fs->aid);
  }
}

static void
fs_descend (fs_t* fs,
	    fs_nodeid_t nodeid,
	    const char* begin,
	    const char* end,
	    descend_callback_t callback,
	    void* arg)
{
  /* TODO:  What if we aren't bound? */

  fs_op_t* op = malloc (sizeof (fs_op_t));
  memset (op, 0, sizeof (fs_op_t));
  op->type = FS_DESCEND;
  op->u.descend.nodeid = nodeid;
  op->u.descend.begin = begin;
  op->u.descend.end = end;
  op->u.descend.callback = callback;
  op->u.descend.arg = arg;
  fs_op_push (fs, op);
}

static void
fs_readfile (fs_t* fs,
	     fs_nodeid_t nodeid,
	     readfile_callback_t callback,
	     void* arg)
{
  /* TODO:  What if we aren't bound? */

  fs_op_t* op = malloc (sizeof (fs_op_t));
  memset (op, 0, sizeof (fs_op_t));
  op->type = FS_READFILE;
  op->u.readfile.nodeid = nodeid;
  op->u.readfile.callback = callback;
  op->u.readfile.arg = arg;
  fs_op_push (fs, op);
}

typedef struct {
  vfs_t* vfs;
  char* path;
  vnode_t vnode;
  const char* begin;
  readfile_callback_t callback;
  void* arg;
} readfile_context_t;

static readfile_context_t*
create_readfile_context (vfs_t* vfs,
			 const char* path,
			 readfile_callback_t callback,
			 void* arg)
{
  size_t path_size = strlen (path) + 1;
  readfile_context_t* c = malloc (sizeof (readfile_context_t));
  memset (c, 0, sizeof (readfile_context_t));
  c->vfs = vfs;
  c->path = malloc (path_size);
  memcpy (c->path, path, path_size);

  /* Start at the top of the tree. */
  c->vnode.aid = -1;
  c->vnode.nodeid = -1;
  c->begin = c->path;
  c->callback = callback;
  c->arg = arg;

  return c;
}

static void
destroy_readfile_context (readfile_context_t* c)
{
  free (c->path);
  free (c);
}

static void
readfile_readfile (void* arg,
		   fs_error_t error,
		   size_t size,
		   bd_t bdb)
{
  readfile_context_t* c = arg;
  c->callback (c->arg, error, size, bdb);
  destroy_readfile_context (c);
}

static void
process_readfile_context (readfile_context_t* c);

static void
readfile_descend (void* arg,
		  fs_error_t error,
		  fs_nodeid_t nodeid)
{
  readfile_context_t* c = arg;
  switch (error) {
  case FS_SUCCESS:
    c->vnode.nodeid = nodeid;
    process_readfile_context (c);
    break;
  default:
    /* TODO */
    break;
  }
}

static void
process_readfile_context (readfile_context_t* c)
{
  if (*c->begin == '\0') {
    fs_t* fs = find_fs (c->vfs, c->vnode.aid);
    fs_readfile (fs, c->vnode.nodeid, readfile_readfile, c);
  }
  else if (*c->begin == '/') {
    ++c->begin;
    /* Redirect. */
    /* TODO: Add a counter to avoid infinite loop. */
    redirect_t* r = c->vfs->redirect_head;;
    while (r != 0) {
      if (vnode_equal (&r->from, &c->vnode)) {
	c->vnode = r->to;
	/* Start over. */
	r = c->vfs->redirect_head;
      }
      else {
	r = r->next;
      }
    }

    fs_t* fs = find_fs (c->vfs, c->vnode.aid);
    
    const char* end = strchr (c->begin, '/');
    if (end == NULL) {
      end = c->begin + strlen (c->begin);
    }
    
    fs_descend (fs, c->vnode.nodeid, c->begin, end, readfile_descend, c);
    c->begin = end;
  }
  else {
    /* TODO */ 
    logs ("BAD PATH\n");
  }
}

void
vfs_init (vfs_t* vfs,
	  ano_t request,
	  ano_t response)
{
  vfs->request = request;
  vfs->response = response;
  vfs->redirect_tail = &vfs->redirect_head;
}

int
vfs_append (vfs_t* vfs,
	    aid_t from_aid,
	    fs_nodeid_t from_nodeid,
	    aid_t to_aid,
	    fs_nodeid_t to_nodeid)
{
  /* Do we have a file system for to_aid? */
  fs_t* to_fs = find_fs (vfs, to_aid);
  if (to_fs == 0) {
    to_fs = create_fs (vfs, to_aid);
  }
  fs_incref (to_fs);

  /* Create the redirect. */
  redirect_t* r = malloc (sizeof (redirect_t));
  memset (r, 0, sizeof (redirect_t));
  r->from.aid = from_aid;
  r->from.nodeid = from_nodeid;
  r->to.aid = to_aid;
  r->to.nodeid = to_nodeid;

  /* Insert into the list. */
  *vfs->redirect_tail = r;
  vfs->redirect_tail = &r->next;

  if (to_fs->bound) {
    return 0;
  }
  else {
    return -1;
  }
}

void
vfs_readfile (vfs_t* vfs,
	      const char* path,
	      readfile_callback_t callback,
	      void* arg)
{
  readfile_context_t* c = create_readfile_context (vfs, path, callback, arg);
  process_readfile_context (c);
}

void
vfs_schedule (const vfs_t* vfs)
{
  /* Schedule file systems with an op. */
  for (fs_t* fs = vfs->fs_head; fs != 0; fs = fs->next) {
    fs_schedule (vfs, fs);
  }
}

void
vfs_request (vfs_t* vfs,
	     aid_t aid)
{
  fs_t* fs = find_fs (vfs, aid);

  if (fs != 0) {
    fs_request (fs);
  }
  else {
    finish_output (false, -1, -1);
  }
}

void
vfs_response (vfs_t* vfs,
	      aid_t aid,
	      bd_t bda,
	      bd_t bdb)
{
  fs_t* fs = find_fs (vfs, aid);

  if (fs != 0) {
    fs_response (fs, bda, bdb);
  }
  else {
    finish_input (bda, bdb);
  }
}

int
fs_request_type_read (buffer_file_t* bf,
		      fs_type_t* type)
{
  if (buffer_file_read (bf, type, sizeof (fs_type_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_descend_request_write (buffer_file_t* bf,
			  fs_nodeid_t nodeid,
			  const char* begin,
			  size_t size)
{
  fs_type_t type = FS_DESCEND;
  if (buffer_file_write (bf, &type, sizeof (fs_type_t)) != 0 ||
      buffer_file_write (bf, &nodeid, sizeof (fs_nodeid_t)) != 0 ||
      buffer_file_write (bf, &size, sizeof (size_t)) != 0 ||
      buffer_file_write (bf, begin, size) != 0) {
    return -1;
  }

  return 0;
}

int
fs_descend_request_read (buffer_file_t* bf,
			 fs_nodeid_t* nodeid,
			 const char** begin,
			 size_t* size)
{
  if (buffer_file_read (bf, nodeid, sizeof (fs_nodeid_t)) != 0 ||
      buffer_file_read (bf, size, sizeof (size_t)) != 0) {
    return -1;
  }

  *begin = buffer_file_readp (bf, *size);
  if (*begin == 0) {
    return -1;
  }

  return 0;
}

int
fs_descend_response_write (buffer_file_t* bf,
			   fs_error_t error,
			   fs_nodeid_t nodeid)
{
  if (buffer_file_write (bf, &error, sizeof (fs_error_t)) != 0 ||
      buffer_file_write (bf, &nodeid, sizeof (fs_nodeid_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_descend_response_read (buffer_file_t* bf,
			  fs_error_t* error,
			  fs_nodeid_t* nodeid)
{
  if (buffer_file_read (bf, error, sizeof (fs_error_t)) != 0 ||
      buffer_file_read (bf, nodeid, sizeof (fs_nodeid_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_readfile_request_write (buffer_file_t* bf,
			   fs_nodeid_t nodeid)
{
  fs_type_t type = FS_READFILE;
  if (buffer_file_write (bf, &type, sizeof (fs_type_t)) != 0 ||
      buffer_file_write (bf, &nodeid, sizeof (fs_nodeid_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_readfile_request_read (buffer_file_t* bf,
			  fs_nodeid_t* nodeid)
{
  if (buffer_file_read (bf, nodeid, sizeof (fs_nodeid_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_readfile_response_write (buffer_file_t* bf,
			    fs_error_t error,
			    size_t size)
{
  if (buffer_file_write (bf, &error, sizeof (fs_error_t)) != 0 ||
      buffer_file_write (bf, &size, sizeof (size_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_readfile_response_read (buffer_file_t* bf,
			   fs_error_t* error,
			   size_t* size)
{
  if (buffer_file_read (bf, error, sizeof (fs_error_t)) != 0 ||
      buffer_file_read (bf, size, sizeof (size_t)) != 0) {
    return -1;
  }

  return 0;
}
