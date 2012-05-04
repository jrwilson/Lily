#include "fs_set.h"
#include <dymem.h>
#include <string.h>
#include <automaton.h>
#include <description.h>

/* TODO: Error handling. */

typedef void (*descend_callback_t) (void* arg, const fs_descend_response_t* res);

typedef struct descend_request descend_request_t;
struct descend_request {
  fs_descend_request_t* request;
  descend_callback_t callback;
  void* arg;
  descend_request_t* next;
};

typedef struct readfile_request readfile_request_t;
struct readfile_request {
  fs_readfile_request_t* request;
  readfile_callback_t callback;
  void* arg;
  readfile_request_t* next;
};

struct fs {
  fs_set_t* fs_set;
  aid_t aid;
  char* name;
  fs_nodeid_t nodeid;
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

static fs_t*
find_fs_aid (const fs_set_t* fs_set,
	     aid_t aid)
{
  fs_t* fs;
  for (fs = fs_set->fs_head; fs != 0; fs = fs->next) {
    if (fs->aid == aid) {
      break;
    }
  }
  
  return fs;
}

static fs_t*
find_fs_name (const fs_set_t* fs_set,
	      const char* name)
{
  fs_t* fs;
  for (fs = fs_set->fs_head; fs != 0; fs = fs->next) {
    if (strcmp (fs->name, name) == 0) {
      break;
    }
  }
  
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
  return fs->descend_request_head != 0 && bind_stat_output_bound (fs->fs_set->bs, fs->fs_set->descend_request, fs->aid) && bind_stat_input_bound (fs->fs_set->bs, fs->fs_set->descend_response, fs->aid);
}

static void
fs_descend_request (fs_t* fs)
{
  if (fs_descend_request_precondition (fs)) {
    buffer_file_shred (fs->fs_set->output_bfa);
    fs_descend_request_write (fs->fs_set->output_bfa, fs->descend_request_head->request);
    shift_descend_request (fs);
    finish_output (true, fs->fs_set->output_bfa->bd, -1);
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
		       const char* path_begin,
		       const char* path_end,
		       readfile_callback_t callback,
		       void* arg)
{
  readfile_request_t* req = malloc (sizeof (readfile_request_t));
  memset (req, 0, sizeof (readfile_request_t));
  req->request = fs_readfile_request_create (nodeid, path_begin, path_end);
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
  fs_readfile_request_destroy (req->request);
  free (req);
}

static bool
fs_readfile_request_precondition (const fs_t* fs)
{
  return fs->readfile_request_head != 0 && bind_stat_output_bound (fs->fs_set->bs, fs->fs_set->readfile_request, fs->aid) && bind_stat_input_bound (fs->fs_set->bs, fs->fs_set->readfile_response, fs->aid);
}

static void
fs_readfile_request (fs_t* fs)
{
  if (fs_readfile_request_precondition (fs)) {
    buffer_file_shred (fs->fs_set->output_bfa);
    fs_readfile_request_write (fs->fs_set->output_bfa, fs->readfile_request_head->request);
    shift_readfile_request (fs);
    finish_output (true, fs->fs_set->output_bfa->bd, -1);
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

  fs->readfile_response_head->callback (fs->readfile_response_head->arg, res, bdb);

  pop_readfile_response (fs);

  finish_input (bda, bdb);
}

static void
fs_schedule (const fs_t* fs)
{
  if (fs_descend_request_precondition (fs)) {
    schedule (fs->fs_set->descend_request, fs->aid);
  }
  if (fs_readfile_request_precondition (fs)) {
    schedule (fs->fs_set->readfile_request, fs->aid);
  }
}

void
fs_set_init (fs_set_t* vfs,
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
}

void
fs_set_insert (fs_set_t* fs_set,
	       aid_t aid,
	       const char* name,
	       fs_nodeid_t nodeid)
{
  /* Do we have a file system for to_aid? */
  if (find_fs_aid (fs_set, aid) != 0 ||
      find_fs_name (fs_set, name) != 0) {
    /* Already exists. */
    return;
  }

  /* Create the file system. */
  fs_t* fs = malloc (sizeof (fs_t));
  memset (fs, 0, sizeof (fs_t));
  fs->fs_set = fs_set;
  fs->aid = aid;
  size_t size = strlen (name) + 1;
  fs->name = malloc (size);
  memcpy (fs->name, name, size);
  fs->nodeid = nodeid;
  fs->descend_request_head = 0;
  fs->descend_request_tail = &fs->descend_request_head;
  fs->descend_response_head = 0;
  fs->descend_response_tail = &fs->descend_response_head;
  fs->readfile_request_head = 0;
  fs->readfile_request_tail = &fs->readfile_request_head;
  fs->readfile_response_head = 0;
  fs->readfile_response_tail = &fs->readfile_response_head;
  
  /* Insert into the list. */
  fs->next = fs_set->fs_head;
  fs_set->fs_head = fs;
  
  /* Bind. */
  automaton_t* this_a = system_get_this (fs_set->system);
  automaton_t* fs_a = system_add_unmanaged_automaton (fs_set->system, aid);
  system_add_binding (fs_set->system,
  		      this_a, 0, 0, fs_set->descend_request, 0,
  		      fs_a, FS_DESCEND_REQUEST_IN_NAME, 0, -1, 0,
  		      this_a);
  system_add_binding (fs_set->system,
  		      fs_a, FS_DESCEND_RESPONSE_OUT_NAME, 0, -1, 0,
  		      this_a, 0, 0, fs_set->descend_response, 0,
  		      this_a);
  system_add_binding (fs_set->system,
  		      this_a, 0, 0, fs_set->readfile_request, 0,
  		      fs_a, FS_READFILE_REQUEST_IN_NAME, 0, -1, 0,
  		      this_a);
  system_add_binding (fs_set->system,
  		      fs_a, FS_READFILE_RESPONSE_OUT_NAME, 0, -1, 0,
  		      this_a, 0, 0, fs_set->readfile_response, 0,
  		      this_a);
}

void
fs_set_make_active (fs_set_t* fs_set,
		    aid_t aid)
{
  fs_t* fs = find_fs_aid (fs_set, aid);
  if (fs != 0) {
    fs_set->active_fs = fs;
  }
}

void
fs_set_readfile (fs_set_t* fs_set,
		 const char* path_begin,
		 const char* path_end,
		 readfile_callback_t callback,
		 void* arg)
{
  if (fs_set->active_fs != 0) {
    push_readfile_request (fs_set->active_fs, fs_set->active_fs->nodeid, path_begin, path_end, callback, arg);
  }
}

void
fs_set_descend_request (fs_set_t* fs_set,
			aid_t aid)
{
  for (fs_t* fs = fs_set->fs_head; fs != 0; fs = fs->next) {
    if (fs->aid == aid) {
      fs_descend_request (fs);
      break;
    }
  }
  
  finish_output (false, -1, -1);
}

void
fs_set_descend_response (fs_set_t* fs_set,
			 aid_t aid,
			 bd_t bda,
			 bd_t bdb)
{
  fs_t* fs = find_fs_aid (fs_set, aid);

  if (fs != 0) {
    fs_descend_response (fs, bda, bdb);
  }
  else {
    finish_input (bda, bdb);
  }
}

void
fs_set_readfile_request (fs_set_t* fs_set,
			 aid_t aid)
{
  for (fs_t* fs = fs_set->fs_head; fs != 0; fs = fs->next) {
    if (fs->aid == aid) {
      fs_readfile_request (fs);
      break;
    }
  }

  finish_output (false, -1, -1);
}

void
fs_set_readfile_response (fs_set_t* fs_set,
			  aid_t aid,
			  bd_t bda,
			  bd_t bdb)
{
  fs_t* fs = find_fs_aid (fs_set, aid);

  if (fs != 0) {
    fs_readfile_response (fs, bda, bdb);
  }
  else {
    finish_input (bda, bdb);
  }
}

void
fs_set_schedule (const fs_set_t* fs_set)
{
  for (fs_t* fs = fs_set->fs_head; fs != 0; fs = fs->next) {
    fs_schedule (fs);
  }
}

