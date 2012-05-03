#include "vfs.h"
#include <dymem.h>
#include <string.h>
#include <automaton.h>
#include <description.h>

/* TODO: Error handling. */

typedef void (*descend_callback_t) (void* arg, fs_error_t error, fs_nodeid_t nodeid);

struct descend_request {
  aid_t to;
  fs_descend_request_t* request;
  descend_request_t* next;
};

/* typedef struct fs_op fs_op_t; */
/* struct fs_op { */
/*   fs_type_t type; */
/*   union { */
/*     struct { */
/*       fs_nodeid_t nodeid; */
/*       const char* begin; */
/*       const char* end; */
/*       descend_callback_t callback; */
/*       void* arg; */
/*     } descend; */
/*     struct { */
/*       fs_nodeid_t nodeid; */
/*       readfile_callback_t callback; */
/*       void* arg; */
/*     } readfile; */
/*   } u; */
/*   fs_op_t* next; */
/* }; */

struct fs {
  size_t refcount;
  vfs_t* vfs;
  aid_t aid;
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
  fs->vfs = vfs;
  fs->aid = aid;

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
fs_incref (fs_t* fs)
{
  ++fs->refcount;
}

/* static void */
/* fs_decref (fs_t* fs) */
/* { */
/*   --fs->refcount; */
/*   if (fs->refcount == 0) { */
/*     buffer_destroy (fs->request_bda); */
/*     buffer_destroy (fs->request_bdb); */
/*     /\* TODO *\/ */
/*     logs (__func__); */
/*     /\* unbind (fs->response_bid); *\/ */
/*     /\* unbind (fs->request_bid); *\/ */

/*     for (fs_t** ptr = &fs->vfs->fs_head; *ptr != 0; ptr = &(*ptr)->next) { */
/*       if (*ptr == fs) { */
/* 	*ptr = fs->next; */
/* 	break; */
/*       } */
/*     } */
    
/*     free (fs); */
/*   } */
/* } */

/* static void */
/* fs_op_push (fs_t* fs, */
/* 	    fs_op_t* op) */
/* { */
/*   *fs->op_tail = op; */
/*   fs->op_tail = &op->next; */
/*   fs_incref (fs); */
/* } */

/* static void */
/* fs_op_pop (fs_t* fs) */
/* { */
/*   fs_op_t* op = fs->op_head; */
/*   fs->op_head = op->next; */
/*   if (fs->op_head == 0) { */
/*     fs->op_tail = &fs->op_head; */
/*   } */
/*   free (op); */
/*   fs_decref (fs); */
/* } */

/* static bool */
/* fs_op_empty (const fs_t* fs) */
/* { */
/*   return fs->op_head == 0; */
/* } */

/* static bool */
/* fs_request_precondition (const fs_t* fs) */
/* { */
/*   return !fs->sent && !fs_op_empty (fs) && bind_stat_output_bound (fs->vfs->bs, fs->vfs->request, 0) && bind_stat_input_bound (fs->vfs->bs, fs->vfs->response, 0); */
/* } */

/* static void */
/* fs_request (fs_t* fs) */
/* { */
/*   if (fs_request_precondition (fs)) { */
/*       buffer_file_truncate (&fs->request_buffer); */
/*       /\* TODO *\/ */
/*       buffer_resize (fs->request_bdb, 0); */
    
/*       fs_op_t* op = fs->op_head; */
/*       switch (op->type) { */
/*       case FS_DESCEND: */
/*         /\* TODO *\/ */
/*         fs_descend_request_write (&fs->request_buffer, op->u.descend.nodeid, op->u.descend.begin, op->u.descend.end - op->u.descend.begin); */
/*         break; */
/*       case FS_READFILE: */
/*         /\* TODO *\/ */
/*         fs_readfile_request_write (&fs->request_buffer, op->u.readfile.nodeid); */
/*         break; */
/*       } */
    
/*       fs->sent = true; */
/*       finish_output (true, fs->request_bda, fs->request_bdb); */
/*   } */
/*   finish_output (false, -1, -1); */
/* } */

/* static void */
/* fs_response (fs_t* fs, */
/* 	     bd_t bda, */
/* 	     bd_t bdb) */
/* { */
/*   if (!fs->sent) { */
/*     /\* Response when none was expected. *\/ */
/*     finish_input (bda, bdb); */
/*   } */

/*   buffer_file_t buffer; */
/*   if (buffer_file_initr (&buffer, bda) != 0) { */
/*     /\* TODO *\/ */
/*     finish_input (bda, bdb); */
/*   }     */

/*   fs_op_t* op = fs->op_head; */
/*   switch (op->type) { */
/*   case FS_DESCEND: */
/*     { */
/*       fs_error_t error; */
/*       fs_nodeid_t nodeid; */
/*       if (fs_descend_response_read (&buffer, &error, &nodeid) == 0) { */
/* 	op->u.descend.callback (op->u.descend.arg, error, nodeid); */
/*       } */
/*       else { */
/* 	/\* TODO *\/ */
/*       } */
/*     } */
/*     break; */
/*   case FS_READFILE: */
/*   { */
/*       fs_error_t error; */
/*       size_t size; */
/*       if (fs_readfile_response_read (&buffer, &error, &size) == 0 && */
/* 	  buffer_size (bdb) >= size_to_pages (size)) { */
/* 	op->u.readfile.callback (op->u.readfile.arg, error, size, bdb); */
/*       } */
/*       else { */
/* 	/\* TODO *\/ */
/*       } */
/*     } */
/*       break; */
/*   } */
  
/*   fs_op_pop (fs); */
/*   fs->sent = false; */

/*   finish_input (bda, bdb); */
/* } */

/* static void */
/* fs_schedule (const fs_t* fs) */
/* { */
/*   if (fs_request_precondition (fs)) { */
/*     schedule (fs->vfs->request, fs->aid); */
/*   } */
/* } */

static void
push_descend_request (vfs_t* vfs,
		      aid_t to,
		      fs_nodeid_t nodeid,
		      const char* name_begin,
		      const char* name_end)
{
  descend_request_t* req = malloc (sizeof (descend_request_t));
  memset (req, 0, sizeof (descend_request_t));
  req->to = to;
  req->request = fs_descend_request_create (nodeid, name_begin, name_end);
  
  *vfs->descend_request_tail = req;
  vfs->descend_request_tail = &req->next;
}

static void
shift_descend_request (vfs_t* vfs)
{
  descend_request_t* req = vfs->descend_request_head;
  vfs->descend_request_head = req->next;
  if (vfs->descend_request_head == 0) {
    vfs->descend_request_tail = &vfs->descend_request_head;
  }
  req->next = 0;

  *vfs->descend_response_tail = req;
  vfs->descend_response_tail = &req->next;
}

static void
push_readfile_request (vfs_t* vfs)
{
  logs (__func__);
}

/* static void */
/* fs_descend (fs_t* fs, */
/* 	    fs_nodeid_t nodeid, */
/* 	    const char* begin, */
/* 	    const char* end, */
/* 	    descend_callback_t callback, */
/* 	    void* arg) */
/* { */
/*   /\* TODO:  What if we aren't bound? *\/ */

/*   fs_op_t* op = malloc (sizeof (fs_op_t)); */
/*   memset (op, 0, sizeof (fs_op_t)); */
/*   op->type = FS_DESCEND; */
/*   op->u.descend.nodeid = nodeid; */
/*   op->u.descend.begin = begin; */
/*   op->u.descend.end = end; */
/*   op->u.descend.callback = callback; */
/*   op->u.descend.arg = arg; */
/*   fs_op_push (fs, op); */
/* } */

/* static void */
/* fs_readfile (fs_t* fs, */
/* 	     fs_nodeid_t nodeid, */
/* 	     readfile_callback_t callback, */
/* 	     void* arg) */
/* { */
/*   /\* TODO:  What if we aren't bound? *\/ */

/*   fs_op_t* op = malloc (sizeof (fs_op_t)); */
/*   memset (op, 0, sizeof (fs_op_t)); */
/*   op->type = FS_READFILE; */
/*   op->u.readfile.nodeid = nodeid; */
/*   op->u.readfile.callback = callback; */
/*   op->u.readfile.arg = arg; */
/*   fs_op_push (fs, op); */
/* } */

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

/* static void */
/* destroy_readfile_context (readfile_context_t* c) */
/* { */
/*   free (c->path); */
/*   free (c); */
/* } */

/* static void */
/* readfile_readfile (void* arg, */
/* 		   fs_error_t error, */
/* 		   size_t size, */
/* 		   bd_t bdb) */
/* { */
/*   readfile_context_t* c = arg; */
/*   c->callback (c->arg, error, size, bdb); */
/*   destroy_readfile_context (c); */
/* } */

static void
process_readfile_context (readfile_context_t* c);

/* static void */
/* readfile_descend (void* arg, */
/* 		  fs_error_t error, */
/* 		  fs_nodeid_t nodeid) */
/* { */
/*   readfile_context_t* c = arg; */
/*   switch (error) { */
/*   case FS_SUCCESS: */
/*     c->vnode.nodeid = nodeid; */
/*     process_readfile_context (c); */
/*     break; */
/*   default: */
/*     c->callback (c->arg, error, -1, -1); */
/*     destroy_readfile_context (c); */
/*     break; */
/*   } */
/* } */

static void
process_readfile_context (readfile_context_t* c)
{
  if (*c->begin == '\0') {
    //fs_t* fs = find_fs (c->vfs, c->vnode.aid);
    push_readfile_request (c->vfs);
    //fs_readfile (fs, c->vnode.nodeid, readfile_readfile, c);
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

    //fs_t* fs = find_fs (c->vfs, c->vnode.aid);
    
    const char* end = strchr (c->begin, '/');
    if (end == NULL) {
      end = c->begin + strlen (c->begin);
    }
    
    push_descend_request (c->vfs, c->vnode.aid, c->vnode.nodeid, c->begin, end);
    //fs_descend (fs, c->vnode.nodeid, c->begin, end, readfile_descend, c);
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
  vfs->descend_request_head = 0;
  vfs->descend_request_tail = &vfs->descend_request_head;
  vfs->descend_response_head = 0;
  vfs->descend_response_tail = &vfs->descend_response_head;
  vfs->readfile_request = readfile_request;
  vfs->readfile_response = readfile_response;
  vfs->readfile_head = 0;
  vfs->readfile_tail = &vfs->readfile_head;
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

static bool descend_request_precondition (const vfs_t* vfs)
{
  if (vfs->descend_request_head != 0) {
    aid_t to = vfs->descend_request_head->to;
    return bind_stat_output_bound (vfs->bs, vfs->descend_request, to) && bind_stat_input_bound (vfs->bs, vfs->descend_response, to);
  }
  else {
    return false;
  }
}

void
vfs_descend_request (vfs_t* vfs,
		     aid_t aid)
{
  if (descend_request_precondition (vfs) && vfs->descend_request_head->to == aid) {
    buffer_file_shred (vfs->output_bfa);
    fs_descend_request_write (vfs->output_bfa, vfs->descend_request_head->request);
    shift_descend_request (vfs);
    finish_output (true, vfs->output_bfa->bd, -1);
  }
  finish_output (false, -1, -1);
}

void
vfs_descend_response (vfs_t* vfs,
		      aid_t aid,
		      bd_t bda,
		      bd_t bdb)
{
  logs (__func__);
/*   fs_t* fs = find_fs (vfs, aid); */

/*   if (fs != 0) { */
/*     fs_response (fs, bda, bdb); */
/*   } */
/*   else { */
/*     finish_input (bda, bdb); */
/*   } */
  finish_input (bda, bdb);
}

void
vfs_schedule (const vfs_t* vfs)
{
  if (descend_request_precondition (vfs)) {
    schedule (vfs->descend_request, vfs->descend_request_head->to);
  }
}

