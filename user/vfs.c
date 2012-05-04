#include "vfs.h"
#include <dymem.h>
#include <string.h>
#include <automaton.h>
#include <description.h>

/* TODO: Error handling. */

typedef void (*descend_callback_t) (void* arg, const fs_descend_response_t* res);

struct descend_request {
  fs_descend_request_t* request;
  descend_callback_t callback;
  void* arg;
  descend_request_t* next;
};

struct readfile_request {
  fs_readfile_request_t request;
  readfile_callback_t callback;
  void* arg;
  readfile_request_t* next;
};

struct fs {
  vfs_t* vfs;
  aid_t aid;
  descend_request_t* descend_request_head;
  descend_request_t** descend_request_tail;
  descend_request_t* descend_response_head;
  descend_request_t** descend_response_tail;
  readfile_request_t* readfile_request_head;
  readfile_request_t** readfile_request_tail;
  readfile_request_t* readfile_response_head;
  readfile_request_t** readfile_response_tail;
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
  fs->vfs = vfs;
  fs->aid = aid;
  fs->descend_request_head = 0;
  fs->descend_request_tail = &fs->descend_request_head;
  fs->descend_response_head = 0;
  fs->descend_response_tail = &fs->descend_response_head;
  fs->readfile_request_head = 0;
  fs->readfile_request_tail = &fs->readfile_request_head;
  fs->readfile_response_head = 0;
  fs->readfile_response_tail = &fs->readfile_response_head;

  /* Insert into the list. */
  fs->next = vfs->fs_head;
  vfs->fs_head = fs;

  /* Bind. */
  automaton_t* this_a = system_get_this (vfs->system);
  automaton_t* fs_a = system_add_unmanaged_automaton (vfs->system, aid);
  system_add_binding (vfs->system,
  		      this_a, 0, 0, vfs->descend_request, 0,
  		      fs_a, FS_DESCEND_REQUEST_IN_NAME, 0, -1, 0,
  		      this_a);
  system_add_binding (vfs->system,
  		      fs_a, FS_DESCEND_RESPONSE_OUT_NAME, 0, -1, 0,
  		      this_a, 0, 0, vfs->descend_response, 0,
  		      this_a);
  system_add_binding (vfs->system,
  		      this_a, 0, 0, vfs->readfile_request, 0,
  		      fs_a, FS_READFILE_REQUEST_IN_NAME, 0, -1, 0,
  		      this_a);
  system_add_binding (vfs->system,
  		      fs_a, FS_READFILE_RESPONSE_OUT_NAME, 0, -1, 0,
  		      this_a, 0, 0, vfs->readfile_response, 0,
  		      this_a);

  return fs;
}

static void
push_descend_request (fs_t* fs,
		      fs_nodeid_t nodeid,
		      const char* name_begin,
		      const char* name_end,
		      descend_callback_t callback,
		      void* arg)
{
  descend_request_t* req = malloc (sizeof (descend_request_t));
  memset (req, 0, sizeof (descend_request_t));
  req->request = fs_descend_request_create (nodeid, name_begin, name_end);
  req->callback = callback;
  req->arg = arg;
  
  *fs->descend_request_tail = req;
  fs->descend_request_tail = &req->next;
}

static void
shift_descend_request (fs_t* fs)
{
  descend_request_t* req = fs->descend_request_head;
  fs->descend_request_head = req->next;
  if (fs->descend_request_head == 0) {
    fs->descend_request_tail = &fs->descend_request_head;
  }
  req->next = 0;

  *fs->descend_response_tail = req;
  fs->descend_response_tail = &req->next;
}

static void
pop_descend_response (fs_t* fs)
{
  descend_request_t* req = fs->descend_response_head;
  fs->descend_response_head = req->next;
  if (fs->descend_response_head == 0) {
    fs->descend_response_tail = &fs->descend_response_head;
  }

  fs_descend_request_destroy (req->request);
  free (req);
}

static bool
fs_descend_request_precondition (const fs_t* fs)
{
  return fs->descend_request_head != 0 && bind_stat_output_bound (fs->vfs->bs, fs->vfs->descend_request, fs->aid) && bind_stat_input_bound (fs->vfs->bs, fs->vfs->descend_response, fs->aid);
}

static void
fs_descend_request (fs_t* fs)
{
  if (fs_descend_request_precondition (fs)) {
    buffer_file_shred (fs->vfs->output_bfa);
    fs_descend_request_write (fs->vfs->output_bfa, fs->descend_request_head->request);
    shift_descend_request (fs);
    finish_output (true, fs->vfs->output_bfa->bd, -1);
  }

  finish_output (false, -1, -1);
}

static void
fs_descend_response (fs_t* fs,
		     bd_t bda,
		     bd_t bdb)
{
  if (fs->descend_response_head == 0) {
    /* TODO:  log */
    finish_input (bda, bdb);
  }
  
  buffer_file_t bf;
  if (buffer_file_initr (&bf, bda) != 0) {
    /* TODO:  log */
    finish_input (bda, bdb);
  }

  const fs_descend_response_t* res = buffer_file_readp (&bf, sizeof (fs_descend_response_t));
  if (res == 0) {
    /* TODO:  log */
    finish_input (bda, bdb);
  }

  fs->descend_response_head->callback (fs->descend_response_head->arg, res);

  pop_descend_response (fs);

  finish_input (bda, bdb);
}

static void
push_readfile_request (fs_t* fs,
		       fs_nodeid_t nodeid,
		       readfile_callback_t callback,
		       void* arg)
{
  readfile_request_t* req = malloc (sizeof (readfile_request_t));
  memset (req, 0, sizeof (readfile_request_t));
  fs_readfile_request_init (&req->request, nodeid);
  req->callback = callback;
  req->arg = arg;
  
  *fs->readfile_request_tail = req;
  fs->readfile_request_tail = &req->next;
}

static void
shift_readfile_request (fs_t* fs)
{
  readfile_request_t* req = fs->readfile_request_head;
  fs->readfile_request_head = req->next;
  if (fs->readfile_request_head == 0) {
    fs->readfile_request_tail = &fs->readfile_request_head;
  }
  req->next = 0;

  *fs->readfile_response_tail = req;
  fs->readfile_response_tail = &req->next;
}

static void
pop_readfile_response (fs_t* fs)
{
  readfile_request_t* req = fs->readfile_response_head;
  fs->readfile_response_head = req->next;
  if (fs->readfile_response_head == 0) {
    fs->readfile_response_tail = &fs->readfile_response_head;
  }
  free (req);
}

static bool
fs_readfile_request_precondition (const fs_t* fs)
{
  return fs->readfile_request_head != 0 && bind_stat_output_bound (fs->vfs->bs, fs->vfs->readfile_request, fs->aid) && bind_stat_input_bound (fs->vfs->bs, fs->vfs->readfile_response, fs->aid);
}

static void
fs_readfile_request (fs_t* fs)
{
  if (fs_readfile_request_precondition (fs)) {
    buffer_file_shred (fs->vfs->output_bfa);
    buffer_file_write (fs->vfs->output_bfa, &fs->readfile_request_head->request, sizeof (fs_readfile_request_t));
    shift_readfile_request (fs);
    finish_output (true, fs->vfs->output_bfa->bd, -1);
  }

  finish_output (false, -1, -1);
}

static void
fs_readfile_response (fs_t* fs,
		      bd_t bda,
		      bd_t bdb)
{
  if (fs->readfile_response_head == 0) {
    /* TODO:  log */
    finish_input (bda, bdb);
  }
  
  buffer_file_t bf;
  if (buffer_file_initr (&bf, bda) != 0) {
    /* TODO:  log */
    finish_input (bda, bdb);
  }

  const fs_readfile_response_t* res = buffer_file_readp (&bf, sizeof (fs_readfile_response_t));
  if (res == 0) {
    /* TODO:  log */
    finish_input (bda, bdb);
  }

  if (res->error == FS_READFILE_SUCCESS && buffer_size (bdb) < size_to_pages (res->size)) {
    /* TODO:  log */
    finish_input (bda, bdb);
  }

  fs->readfile_response_head->callback (fs->readfile_response_head->arg, 0, res, bdb);

  pop_readfile_response (fs);

  finish_input (bda, bdb);
}

static void
fs_schedule (const fs_t* fs)
{
  if (fs_descend_request_precondition (fs)) {
    schedule (fs->vfs->descend_request, fs->aid);
  }
  if (fs_readfile_request_precondition (fs)) {
    schedule (fs->vfs->readfile_request, fs->aid);
  }
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
			 const char* path_begin,
			 const char* path_end,
			 readfile_callback_t callback,
			 void* arg)
{
  size_t path_size = path_end - path_begin + 1;
  readfile_context_t* c = malloc (sizeof (readfile_context_t));
  memset (c, 0, sizeof (readfile_context_t));
  c->vfs = vfs;
  c->path = malloc (path_size);
  memcpy (c->path, path_begin, path_size - 1);
  c->path[path_size - 1] = '\0';

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
		   const fs_descend_response_t* res1,
		   const fs_readfile_response_t* res2,
		   bd_t bdb)
{
  readfile_context_t* c = arg;
  c->callback (c->arg, res1, res2, bdb);
  destroy_readfile_context (c);
}

static void
process_readfile_context (readfile_context_t* c);

static void
readfile_descend (void* arg,
		  const fs_descend_response_t* res)
{
  readfile_context_t* c = arg;
  switch (res->error) {
  case FS_DESCEND_SUCCESS:
    c->vnode.nodeid = res->nodeid;
    process_readfile_context (c);
    break;
  default:
    c->callback (c->arg, res, 0, -1);
    destroy_readfile_context (c);
    break;
  }
}

static void
process_readfile_context (readfile_context_t* c)
{
  if (*c->begin == '\0') {
    fs_t* fs = find_fs (c->vfs, c->vnode.aid);
    push_readfile_request (fs, c->vnode.nodeid, readfile_readfile, c);
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
    
    const char* end = strchr (c->begin, '/');
    if (end == NULL) {
      end = c->begin + strlen (c->begin);
    }
    
    fs_t* fs = find_fs (c->vfs, c->vnode.aid);

    push_descend_request (fs, c->vnode.nodeid, c->begin, end, readfile_descend, c);
    c->begin = end;
  }
  else {
    /* TODO */
    logs ("BAD PATH\n");
  }
}

void
vfs_init (vfs_t* vfs,
	  system_t* system,
	  bind_stat_t* bs,
	  buffer_file_t* output_bfa,
	  ano_t descend_request,
	  ano_t descend_response,
	  ano_t readfile_request,
	  ano_t readfile_response)
{
  vfs->system = system;
  vfs->bs = bs;
  vfs->output_bfa = output_bfa;
  vfs->descend_request = descend_request;
  vfs->descend_response = descend_response;
  vfs->readfile_request = readfile_request;
  vfs->readfile_response = readfile_response;
  vfs->redirect_tail = &vfs->redirect_head;
}

void
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
}

void
vfs_readfile (vfs_t* vfs,
	      const char* path_begin,
	      const char* path_end,
	      readfile_callback_t callback,
	      void* arg)
{
  readfile_context_t* c = create_readfile_context (vfs, path_begin, path_end, callback, arg);
  process_readfile_context (c);
}

void
vfs_descend_request (vfs_t* vfs,
		     aid_t aid)
{
  for (fs_t* fs = vfs->fs_head; fs != 0; fs = fs->next) {
    if (fs->aid == aid) {
      fs_descend_request (fs);
      break;
    }
  }

  finish_output (false, -1, -1);
}

void
vfs_descend_response (vfs_t* vfs,
		      aid_t aid,
		      bd_t bda,
		      bd_t bdb)
{
  fs_t* fs = find_fs (vfs, aid);

  if (fs != 0) {
    fs_descend_response (fs, bda, bdb);
  }
  else {
    finish_input (bda, bdb);
  }
}

void
vfs_readfile_request (vfs_t* vfs,
		      aid_t aid)
{
  for (fs_t* fs = vfs->fs_head; fs != 0; fs = fs->next) {
    if (fs->aid == aid) {
      fs_readfile_request (fs);
      break;
    }
  }

  finish_output (false, -1, -1);
}

void
vfs_readfile_response (vfs_t* vfs,
		       aid_t aid,
		       bd_t bda,
		       bd_t bdb)
{
  fs_t* fs = find_fs (vfs, aid);

  if (fs != 0) {
    fs_readfile_response (fs, bda, bdb);
  }
  else {
    finish_input (bda, bdb);
  }
}

void
vfs_schedule (const vfs_t* vfs)
{
  for (fs_t* fs = vfs->fs_head; fs != 0; fs = fs->next) {
    fs_schedule (fs);
  }
}

