#include <automaton.h>
#include <buffer_file.h>
#include <dymem.h>
#include <string.h>
#include <description.h>
#include "vfs_msg.h"
#include "callback_queue.h"
#include "kv_parse.h"
#include "de.h"
#include "environment.h"

/* An inode represents an entry in a file system. */
typedef size_t inode_t;

/* A vnode represents an entry in a virtual file system. */
typedef struct {
  aid_t aid;
  inode_t inode;
} vnode_t;

typedef struct fs fs_t;
typedef struct redirect redirect_t;

struct fs {
  size_t refcount;
  aid_t aid;
  bool bound;
  /* request queue */
  /* callback queue */
  fs_t* next;
};

struct redirect {
  vnode_t from;
  vnode_t to;
  fs_t* to_fs;
  redirect_t* next;
};

typedef struct {
  ano_t request;
  ano_t response;
  fs_t* fs_head;
  redirect_t* redirect_head;
  redirect_t** redirect_tail;
} vfs_t;

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
  fs->refcount = 1;
  fs->aid = aid;
  fs->bound = false;

  /* Insert into the list. */
  fs->next = vfs->fs_head;
  vfs->fs_head = fs;

  description_t description;
  if (description_init (&description, aid) != 0) {
    /* Could not describe. */
    return fs;
  }

  action_desc_t request;
  if (description_read_name (&description, &request, FS_REQUEST_NAME) != 0) {
    description_fini (&description);
    return fs;
  }
  
  action_desc_t response;
  if (description_read_name (&description, &response, FS_RESPONSE_NAME) != 0) {
    description_fini (&description);
    return fs;
  }
  
  aid_t this_aid = getaid ();
  if (bind (aid, response.number, 0, this_aid, vfs->response, 0) == -1 ||
      bind (this_aid, vfs->request, 0, aid, request.number, 0) == -1) {
    description_fini (&description);
    return fs;
  }
  
  description_fini (&description);

  fs->bound = true;
  
  return fs;
}

static void
vfs_init (vfs_t* vfs,
	  ano_t request,
	  ano_t response)
{
  vfs->request = request;
  vfs->response = response;
  vfs->redirect_tail = &vfs->redirect_head;
}

static int
vfs_append (vfs_t* vfs,
	    aid_t from_aid,
	    inode_t from_inode,
	    aid_t to_aid,
	    inode_t to_inode)
{
  int retval = 0;

  /* Do we have a file system for to_aid? */
  fs_t* to_fs = find_fs (vfs, to_aid);
  if (to_fs == 0) {
    to_fs = create_fs (vfs, to_aid);
    if (!to_fs->bound) {
      retval = -1;
    }
  }

  /* Create the redirect. */
  redirect_t* r = malloc (sizeof (redirect_t));
  memset (r, 0, sizeof (redirect_t));
  r->from.aid = from_aid;
  r->from.inode = from_inode;
  r->to.aid = to_aid;
  r->to.inode = to_inode;
  r->to_fs = to_fs;

  /* Insert into the list. */
  *vfs->redirect_tail = r;
  vfs->redirect_tail = &r->next;

  return retval;
}

/* TODO:  Improve the error handling. */

/* TODO:  This is wrong on multiple levels. */
static int
atoi (const char* s)
{
  bool negate = false;
  int retval = 0;

  if (*s == '-') {
    negate = true;
  }

  while (*s != 0) {
    switch (*s) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      retval *= 10;
      retval += (*s - '0');
      break;
    default:
      goto out;
    }
    ++s;
  }

 out:

  if (negate) {
    return -retval;
  }
  else {
    return retval;
  }
}

#define INIT_NO 1
#define TEXT_IN_NO 3
#define PROCESS_LINE_NO 4
#define TEXT_OUT_NO 5
#define DESTROYED_NO 6
#define UNBOUND_NO 7
#define VFS_RESPONSE_NO 8
#define VFS_REQUEST_NO 9
#define COM_OUT_NO 12
#define COM_IN_NO 13

/* Initialization flag. */
static bool initialized = false;

/* Virtual file system. */
static vfs_t vfs;

static bd_t text_out_bd = -1;
static buffer_file_t text_out_buffer;

static bd_t vfs_request_bda = -1;
static bd_t vfs_request_bdb = -1;
static vfs_request_queue_t vfs_request_queue;
static callback_queue_t vfs_response_queue;
static bool process_hold = false;

static bd_t com_out_bd = -1;
static buffer_file_t com_out_buffer;

#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "
#define INFO __FILE__ ": info: "

typedef struct automaton automaton_t;
typedef struct automaton_item automaton_item_t;
typedef struct binding binding_t;
typedef struct binding_item binding_item_t;

struct automaton {
  char* name;
  aid_t aid;
  char* path;
  automaton_t* next;
};

struct automaton_item {
  automaton_t* automaton;
  automaton_item_t* next;
};

struct binding {
  bid_t bid;
  automaton_t* output_automaton;
  ano_t output_action;
  char* output_action_name;
  int output_parameter;
  automaton_t* input_automaton;
  ano_t input_action;
  char* input_action_name;
  int input_parameter;
  binding_t* next;
};

struct binding_item {
  binding_t* binding;
  binding_item_t* next;
};

static automaton_t* this_automaton = 0;
static automaton_t* automata_head = 0;
static binding_t* bindings_head = 0;

static automaton_t*
create_automaton (const char* name,
		  aid_t aid,
		  const char* path)
{
  automaton_t* automaton = malloc (sizeof (automaton_t));
  memset (automaton, 0, sizeof (automaton_t));
  automaton->aid = aid;
  size_t size = strlen (name) + 1;
  automaton->name = malloc (size);
  memcpy (automaton->name, name, size);
  size = strlen (path) + 1;
  automaton->path = malloc (size);
  memcpy (automaton->path, path, size);
  
  return automaton;
}

static void
destroy_automaton (automaton_t* automaton)
{
  free (automaton->name);
  free (automaton->path);
  free (automaton);
}

static automaton_t*
find_automaton (const char* name)
{
  automaton_t* automaton;
  for (automaton = automata_head; automaton != 0; automaton = automaton->next) {
    if (strcmp (automaton->name, name) == 0) {
      break;
    }
  }
  return automaton;
}

/* static automaton_t* */
/* find_automaton_aid (aid_t aid) */
/* { */
/*   automaton_t* automaton; */
/*   for (automaton = automata_head; automaton != 0; automaton = automaton->next) { */
/*     if (automaton->aid == aid) { */
/*       break; */
/*     } */
/*   } */
/*   return automaton; */
/* } */

static binding_t*
create_binding (bid_t bid,
		automaton_t* output_automaton,
		ano_t output_action,
		const char* output_action_name,
		int output_parameter,
		automaton_t* input_automaton,
		ano_t input_action,
		const char* input_action_name,
		int input_parameter)
{
  binding_t* binding = malloc (sizeof (binding_t));
  memset (binding, 0, sizeof (binding_t));

  binding->bid = bid;
  binding->output_automaton = output_automaton;
  binding->output_action = output_action;
  size_t size = strlen (output_action_name) + 1;
  binding->output_action_name = malloc (size);
  memcpy (binding->output_action_name, output_action_name, size);
  binding->output_parameter = output_parameter;
  binding->input_automaton = input_automaton;
  binding->input_action = input_action;
  size = strlen (input_action_name) + 1;
  binding->input_action_name = malloc (size);
  memcpy (binding->input_action_name, input_action_name, size);
  binding->input_parameter = input_parameter;

  return binding;
}

static void
destroy_binding (binding_t* binding)
{
  free (binding->output_action_name);
  free (binding->input_action_name);
  free (binding);
}

static void
unbound_ (binding_t** ptr)
{
  if (*ptr != 0) {
    binding_t* binding = *ptr;
    *ptr = binding->next;
    
    bfprintf (&text_out_buffer, "-> %d: (%s, %s, %d) -> (%s, %s, %d) unbound\n", binding->bid, binding->output_automaton->name, binding->output_action_name, binding->output_parameter, binding->input_automaton->name, binding->input_action_name, binding->input_parameter);
    
    destroy_binding (binding);
  }
}

static void
destroyed_ (automaton_t** aptr)
{
  if (*aptr != 0) {
    automaton_t* automaton = *aptr;
    *aptr = automaton->next;

    /* Remove the bindings. */
    binding_t** bptr = &bindings_head;
    while (*bptr != 0) {
      if ((*bptr)->output_automaton == automaton ||
	  (*bptr)->input_automaton == automaton) {
	binding_t* binding = *bptr;
	*bptr = binding->next;

	bfprintf (&text_out_buffer, "-> %d: (%s, %s, %d) -> (%s, %s, %d) unbound\n", binding->bid, binding->output_automaton->name, binding->output_action_name, binding->output_parameter, binding->input_automaton->name, binding->input_action_name, binding->input_parameter);

	destroy_binding (binding);
      }
      else {
	bptr = &(*bptr)->next;
      }
    }

    bfprintf (&text_out_buffer, "-> %d: destroyed\n", automaton->aid);

    destroy_automaton (automaton);
  }
}

/*
  Begin Line Queue Section
  ========================
*/

typedef struct string string_t;
struct string {
  char* str;
  size_t size;
  string_t* next;
};

static string_t* string_head = 0;
static string_t** string_tail = &string_head;

/* static void */
/* string_push_front (char* str, */
/* 		   size_t size) */
/* { */
/*   string_t* s = malloc (sizeof (string_t)); */
/*   s->str = str; */
/*   s->size = size; */
/*   s->next = string_head; */
/*   string_head = s; */

/*   if (s->next == 0) { */
/*     string_tail = &s->next; */
/*   } */
/* } */

static void
string_push_back (char* str,
		  size_t size)
{
  string_t* s = malloc (sizeof (string_t));
  s->str = str;
  s->size = size;
  s->next = 0;
  *string_tail = s;
  string_tail = &s->next;
}

static void
string_front (char** str,
	      size_t* size)
{
  *str = string_head->str;
  *size = string_head->size;
}

static void
string_pop (void)
{
  string_t* s = string_head;
  string_head = s->next;
  if (string_head == 0) {
    string_tail = &string_head;
  }

  free (s);
}

static bool
string_empty (void)
{
  return string_head == 0;
}

static size_t current_string_idx = 0;

static char* current_line = 0;
static size_t current_line_size = 0;
static size_t current_line_capacity = 0;

static void
line_append (char c)
{
  if (current_line_size == current_line_capacity) {
    current_line_capacity = 2 * current_line_capacity + 1;
    current_line = realloc (current_line, current_line_capacity);
  }
  current_line[current_line_size++] = c;
}

static void
line_back (void)
{
  if (current_line_size != 0) {
    --current_line_size;
  }
}

static void
line_reset (void)
{
  current_line_size = 0;
}

/*
  End Line Queue Section
  ======================
*/

/*
  Begin Scanner Section
  =====================
*/

typedef enum {
  SCAN_START,
  SCAN_STRING,
  SCAN_COMMENT,
  SCAN_ERROR,
} scan_state_t;

static scan_state_t scan_state = SCAN_START;
static char* scan_string_copy = 0;
static char** scan_strings = 0;
static size_t scan_strings_size = 0;
static size_t scan_strings_capacity = 0;

/* Tokens

   string - [a-zA-Z0-9_/-=*?!]+

   whitespace - [ \t\0]

   comment - #[^\n]*
 */

static void
scanner_enter_string (char* str)
{
  if (scan_strings_size == scan_strings_capacity) {
    scan_strings_capacity = scan_strings_capacity * 2 + 1;
    scan_strings = realloc (scan_strings, scan_strings_capacity * sizeof (char*));
  }
  scan_strings[scan_strings_size++] = str;
}

static void
scanner_put (char* ptr)
{
  const char c = *ptr;

  switch (scan_state) {
  case SCAN_START:
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9') ||
	(c == '_') ||
	(c == '/') ||
	(c == '-') ||
	(c == '=') ||
	(c == '*') ||
	(c == '?') ||
	(c == '!')) {
      scanner_enter_string (ptr);
      scan_state = SCAN_STRING;
      break;
    }

    switch (c) {
    case '#':
      scan_state = SCAN_COMMENT;
      break;
    case ' ':
    case '\t':
    case 0:
      /* Eat whitespace. */
      scan_state = SCAN_START;
      break;
    default:
      scan_state = SCAN_ERROR;
    }
    break;

  case SCAN_STRING:
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9') ||
	(c == '_') ||
	(c == '/') ||
	(c == '-') ||
	(c == '=') ||
	(c == '*') ||
	(c == '?') ||
	(c == '!')) {
      /* Still scanning a string. */
      scan_state = SCAN_STRING;
      break;
    }

    switch (c) {
    case '#':
      /* Terminate the string. */
      *ptr = 0;
      scan_state = SCAN_COMMENT;
      break;
    case ' ':
    case '\t':
    case 0:
      /* Terminate the string. */
      *ptr = 0;
      scan_state = SCAN_START;
      break;
    default:
      scan_state = SCAN_ERROR;
    }
    break;

  case SCAN_COMMENT:
    /* Do nothing. */
    break;

  case SCAN_ERROR:
    /* Do nothing. */
    break;
  }
}

static int
scanner_process (char* string,
		 size_t size)
{
  scan_state = SCAN_START;
  free (scan_string_copy);
  scan_string_copy = malloc (size);
  memcpy (scan_string_copy, string, size);

  scan_strings_size = 0;

  size_t idx;
  for (idx = 0; idx != size && scan_state != SCAN_ERROR; ++idx) {
    scanner_put (scan_string_copy + idx);
    if (scan_state == SCAN_ERROR) {
      bfprintf (&text_out_buffer, "-> error near: %s\n", string + idx);
      return -1;
    }
  }

  return 0;
}

/*
  End Scanner Section
  ===================
*/

/*
  Begin Dispatch Section
  ======================
*/

typedef struct com_item com_item_t;
struct com_item {
  aid_t aid;
  char* str;
  com_item_t* next;
};

static com_item_t* com_head = 0;
static com_item_t** com_tail = &com_head;

static bool
com_queue_empty (void)
{
  return com_head == 0;
}

static aid_t
com_queue_front (void)
{
  return com_head->aid;
}

static void
com_queue_push (aid_t aid,
		const char* str)
{
  com_item_t* item = malloc (sizeof (com_item_t));
  memset (item, 0, sizeof (com_item_t));
  item->aid = aid;
  size_t len = strlen (str) + 1;
  item->str = malloc (len);
  memcpy (item->str, str, len);
  *com_tail = item;
  com_tail = &item->next;
}

static void
com_queue_pop (void)
{
  com_item_t* item = com_head;
  com_head = item->next;
  if (com_head == 0) {
    com_tail = &com_head;
  }

  free (item->str);
  free (item);
}

static const char* create_name = 0;
static bool create_retain_privilege = false;
static const char* create_register_name = 0;
static const char* create_path = 0;
static size_t create_argv_idx = 0;

static void
create_callback (void* data,
		 bd_t bda,
		 bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read vfs readfile response: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }

  if (error != VFS_SUCCESS) {
    bfprintf (&text_out_buffer, "-> file %s could not be read: %s\n", create_path, vfs_error_string (error));
    return;
  }

  bd_t bd1 = buffer_create (0);
  if (bd1 == -1) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create argument buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }

  buffer_file_t bf;
  if (buffer_file_initw (&bf, bd1) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize argument buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }

  if (buffer_file_puts (&bf, current_line + (scan_strings[create_argv_idx] - scan_string_copy)) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to argument buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }

  aid_t aid = create (bdb, bd1, -1, create_retain_privilege);
  if (aid == -1) {
    buffer_destroy (bd1);
    bfprintf (&text_out_buffer, "-> error: create failed: %s\n", lily_error_string (lily_error));
    return;
  }

  /* if (subscribe_destroyed (aid, DESTROYED_NO) != 0) { */
  /*   buffer_destroy (bd1); */
  /*   bfprintf (&text_out_buffer, "-> error: subscribe failed: %s\n", lily_error_string (lily_error)); */
  /*   return; */
  /* } */

  /* Add to the list of automata we know about. */
  automaton_t* automaton = create_automaton (create_name, aid, create_path);

  automaton->next = automata_head;
  automata_head = automaton;

  buffer_destroy (bd1);
  
  bfprintf (&text_out_buffer, "-> %s = %d\n", scan_strings[0], aid);
}

static void
create_usage (void)
{
  buffer_file_puts (&text_out_buffer, "-> usage: NAME = create [-p -n NAME] create PATH [OPTIONS...]\n");
}

static bool
create_ (void)
{
  if (scan_strings_size >= 1) {
    if (strcmp ("create", scan_strings[0]) == 0) {
      create_usage ();
      return true;
    }
  }
  if (scan_strings_size >= 3 &&
      strcmp ("=", scan_strings[1]) == 0 &&
      strcmp ("create", scan_strings[2]) == 0) {

    create_name = scan_strings[0];

    if (find_automaton (create_name) != 0) {
      bfprintf (&text_out_buffer, "-> error: name %s is taken\n", create_name);
      return true;
    }

    size_t idx = 3;
    
    /* Parse the options. */
    create_retain_privilege = false;
    create_register_name = 0;
    
    for (;;) {
      if (idx >= scan_strings_size) {
	create_usage ();
	return -1;
      }
      
      if (strcmp (scan_strings[idx], "-p") == 0) {
	create_retain_privilege = true;
	++idx;
	continue;
      }
      
      if (strcmp (scan_strings[idx], "-n") == 0) {
	++idx;
	if (idx >= scan_strings_size) {
	  create_usage ();
	  return -1;
	}
	
	create_register_name = scan_strings[idx];
	++idx;
      }
      
      break;
    }

    if (idx >= scan_strings_size) {
      create_usage ();
      return -1;
    }
    create_path = scan_strings[idx];
    create_argv_idx = idx;
    ++idx;

    /* Request the file. */

    vfs_request_queue_push_readfile (&vfs_request_queue, create_path);
    callback_queue_push (&vfs_response_queue, create_callback, 0);

    return true;
  }
  return false;
}

static void
bind_usage (void)
{
  buffer_file_puts (&text_out_buffer, "-> usage: bind [-o OPARAM -i IPARAM] OAID OACTION IAID IACTION\n");
}

static void
single_bind (automaton_t* output_automaton,
	     const char* output_action_name,
	     int output_parameter,
	     automaton_t* input_automaton,
	     const char* input_action_name,
	     int input_parameter)
{
  /* Describe the output and input automaton. */
  description_t output_description;
  description_t input_description;
  if (description_init (&output_description, output_automaton->aid) != 0) {
    buffer_file_puts (&text_out_buffer, "-> error: could not describe output\n");
    return;
  }
  if (description_init (&input_description, input_automaton->aid) != 0) {
    description_fini (&output_description);
    buffer_file_puts (&text_out_buffer, "-> error: could not describe input\n");
    return;
  }
      
  /* Look up the actions. */
  action_desc_t output_action;
  if (description_read_name (&output_description, &output_action, output_action_name) != 0) {
    description_fini (&output_description);
    description_fini (&input_description);
    buffer_file_puts (&text_out_buffer, "-> error: output action does not exist\n");
    return;
  }

  action_desc_t input_action;
  if (description_read_name (&input_description, &input_action, input_action_name) != 0) {
    description_fini (&output_description);
    description_fini (&input_description);
    buffer_file_puts (&text_out_buffer, "-> error: input action does not exist\n");
    return;
  }

  /* Correct the parameters. */
  switch (output_action.parameter_mode) {
  case NO_PARAMETER:
    output_parameter = 0;
    break;
  case PARAMETER:
    break;
  case AUTO_PARAMETER:
    output_parameter = input_automaton->aid;
    break;
  }

  switch (input_action.parameter_mode) {
  case NO_PARAMETER:
    input_parameter = 0;
    break;
  case PARAMETER:
    break;
  case AUTO_PARAMETER:
    input_parameter = output_automaton->aid;
    break;
  }
      
  bid_t bid = bind (output_automaton->aid, output_action.number, output_parameter, input_automaton->aid, input_action.number, input_parameter);
  if (bid == -1) {
    description_fini (&output_description);
    description_fini (&input_description);
    buffer_file_puts (&text_out_buffer, "-> error: bind failed\n");
    return;
  }

  /* if (subscribe_unbound (bid, UNBOUND_NO) != 0) { */
  /*   description_fini (&output_description); */
  /*   description_fini (&input_description); */
  /*   buffer_file_puts (&text_out_buffer, "-> error: subscribe failed\n"); */
  /*   return; */
  /* } */

  binding_t* binding = create_binding (bid, output_automaton, output_action.number, output_action_name, output_parameter, input_automaton, input_action.number, input_action_name, input_parameter);

  binding->next = bindings_head;
  bindings_head = binding;

  description_fini (&output_description);
  description_fini (&input_description);

  bfprintf (&text_out_buffer, "-> %d: (%s, %s, %d) -> (%s, %s, %d)\n", bid, output_automaton->name, output_action_name, output_parameter, input_automaton->name, input_action_name, input_parameter);
}

static void
glob_bind (automaton_t* output_automaton,
	   const char* output_action_name,
	   const char* output_glob,
	   int output_parameter,
	   automaton_t* input_automaton,
	   const char* input_action_name,
	   const char* input_glob,
	   int input_parameter)
{
  /* Describe the output and input automaton. */
  description_t output_description;
  description_t input_description;
  if (description_init (&output_description, output_automaton->aid) != 0) {
    buffer_file_puts (&text_out_buffer, "-> error: could not describe output\n");
    return;
  }
  if (description_init (&input_description, input_automaton->aid) != 0) {
    description_fini (&output_description);
    buffer_file_puts (&text_out_buffer, "-> error: could not describe input\n");
    return;
  }

  size_t output_action_count = description_action_count (&output_description);
  if (output_action_count == -1) {
    description_fini (&output_description);
    description_fini (&input_description);
    buffer_file_puts (&text_out_buffer, "-> error: bad output description\n");
    return;
  }

  size_t input_action_count = description_action_count (&input_description);
  if (output_action_count == -1) {
    description_fini (&output_description);
    description_fini (&input_description);
    buffer_file_puts (&text_out_buffer, "-> error: bad input description\n");
    return;
  }

  action_desc_t* output_actions = malloc (output_action_count * sizeof (action_desc_t));
  action_desc_t* input_actions = malloc (input_action_count * sizeof (action_desc_t));
        
  if (description_read_all (&output_description, output_actions) != 0) {
    free (output_actions);
    free (input_actions);
    description_fini (&output_description);
    description_fini (&input_description);
    buffer_file_puts (&text_out_buffer, "-> error: bad output description\n");
  }

  if (description_read_all (&input_description, input_actions) != 0) {
    free (output_actions);
    free (input_actions);
    description_fini (&output_description);
    description_fini (&input_description);
    buffer_file_puts (&text_out_buffer, "-> error: bad input description\n");
    return;
  }

  /* How many characters must match before the glob? */
  const size_t output_prefix_size = output_glob - output_action_name;
  const size_t output_suffix_size = strlen (output_action_name) - output_prefix_size - 1;
  const size_t input_prefix_size = input_glob - input_action_name;
  const size_t input_suffix_size = strlen (input_action_name) - input_prefix_size - 1;

  for (size_t out_idx = 0; out_idx != output_action_count; ++out_idx) {
    if (output_actions[out_idx].type == LILY_ACTION_OUTPUT &&
	output_actions[out_idx].name_size >= (output_prefix_size + output_suffix_size + 1) &&
	strncmp (output_action_name, output_actions[out_idx].name, output_prefix_size) == 0 &&
	strncmp (output_glob + 1, output_actions[out_idx].name + output_actions[out_idx].name_size - 1 - output_suffix_size, output_suffix_size) == 0) {

      const char* output_name = output_actions[out_idx].name + output_prefix_size;
      size_t output_size = output_actions[out_idx].name_size - 1 - output_suffix_size;

      for (size_t in_idx = 0; in_idx != input_action_count; ++in_idx) {
      	if (input_actions[in_idx].type == LILY_ACTION_INPUT &&
	    input_actions[in_idx].name_size >= (input_prefix_size + input_suffix_size + 1) &&
	    strncmp (input_action_name, input_actions[in_idx].name, input_prefix_size) == 0 &&
	    strncmp (input_glob + 1, input_actions[in_idx].name + input_actions[in_idx].name_size - 1 - input_suffix_size, input_suffix_size) == 0) {

	  const char* input_name = input_actions[in_idx].name + input_prefix_size;
	  size_t input_size = input_actions[in_idx].name_size - 1 - input_suffix_size;

	  if (output_size == input_size && strncmp (output_name, input_name, output_size) == 0) {
	    int output_param = 0;
	    int input_param = 0;

	    switch (output_actions[out_idx].parameter_mode) {
	    case NO_PARAMETER:
	      output_param = 0;
	      break;
	    case PARAMETER:
	      output_param = output_parameter;
	      break;
	    case AUTO_PARAMETER:
	      output_param = input_automaton->aid;
	      break;
	    }

	    switch (input_actions[in_idx].parameter_mode) {
	    case NO_PARAMETER:
	      input_param = 0;
	      break;
	    case PARAMETER:
	      input_param = input_parameter;
	      break;
	    case AUTO_PARAMETER:
	      input_param = output_automaton->aid;
	      break;
	    }

	    bid_t bid = bind (output_automaton->aid, output_actions[out_idx].number, output_param, input_automaton->aid, input_actions[in_idx].number, input_param);
	    if (bid != -1) {
	      /* if (subscribe_unbound (bid, UNBOUND_NO) == 0) { */
	      /* 	binding_t* binding = create_binding (bid, output_automaton, output_actions[out_idx].number, output_actions[out_idx].name, output_param, input_automaton, input_actions[in_idx].number, input_actions[in_idx].name, input_param); */

	      /* 	binding->next = bindings_head; */
	      /* 	bindings_head = binding; */
	      /* } */
	      /* else { */
	      /* 	bid = -1; */
	      /* } */
	    }

	    bfprintf (&text_out_buffer, "-> %d: (%s, %s, %d) -> (%s, %s, %d)\n", bid, output_automaton->name, output_actions[out_idx].name, output_param, input_automaton->name, input_actions[in_idx].name, input_param);
	  }
      	}
      }
    }
  }

  free (output_actions);
  free (input_actions);
      
  description_fini (&output_description);
  description_fini (&input_description);
}

static bool
bind_ (void)
{
  if (scan_strings_size >= 1) {
    if (strcmp (scan_strings[0], "bind") == 0) {
      size_t idx = 1;

      /* Parse the parameters. */
      int output_parameter = 0;
      int input_parameter = 0;
      
      for (;;) {
	if (idx >= scan_strings_size) {
	  bind_usage ();
	  return true;
	}
	
	if (strcmp (scan_strings[idx], "-o") == 0) {
	  ++idx;
	  if (idx >= scan_strings_size) {
	    bind_usage ();
	    return true;
	  }
	  
	  output_parameter = atoi (scan_strings[idx]);
	  ++idx;
	  continue;
	}
	
	if (strcmp (scan_strings[idx], "-i") == 0) {
	  ++idx;
	  if (idx >= scan_strings_size) {
	    bind_usage ();
	    return true;
	  }
	  
	  input_parameter = atoi (scan_strings[idx]);
	  ++idx;
	}
	
	break;
      }
      
      if (idx + 4 != scan_strings_size) {
	bind_usage ();
	return true;
      }
      
      automaton_t* output_automaton = find_automaton (scan_strings[idx]);
      if (output_automaton == 0) {
	bfprintf (&text_out_buffer, "-> error: %s does not refer to a known automaton\n", scan_strings[idx]);
	return true;
      }
      ++idx;
      
      const char* output_action_name = scan_strings[idx++];
      
      automaton_t* input_automaton = find_automaton (scan_strings[idx]);
      if (input_automaton == 0) {
	bfprintf (&text_out_buffer, "-> error: %s does not refer to a known automaton\n", scan_strings[idx]);
	return true;
      }
      ++idx;
      
      const char* input_action_name = scan_strings[idx++];

      /* Are we globbing? */
      const char* output_glob = strchr (output_action_name, '*');
      const char* input_glob = strchr (input_action_name, '*');

      if (!((output_glob == 0 && input_glob == 0) ||
	   (output_glob != 0 && input_glob != 0))) {
	buffer_file_puts (&text_out_buffer, "-> error: glob disagreement\n");
	return true;
      }

      if (output_glob != 0) {
	glob_bind (output_automaton, output_action_name, output_glob, output_parameter, input_automaton, input_action_name, input_glob, input_parameter);
      }
      else {
	single_bind (output_automaton, output_action_name, output_parameter, input_automaton, input_action_name, input_parameter);
      }
      return true;
    }
  }
  return false;
}

static bool
unbind_ (void)
{
  if (scan_strings_size >= 1 && strcmp (scan_strings[0], "unbind") == 0) {
    if (scan_strings_size != 2) {
      buffer_file_puts (&text_out_buffer, "-> usage: unbind BID\n");
      return true;
    }
  
    bid_t bid = atoi (scan_strings[1]);

    binding_t** ptr;
    for (ptr = &bindings_head; *ptr != 0; ptr = &(*ptr)->next) {
      if ((*ptr)->bid == bid) {
	break;
      }
    }

    if (*ptr == 0) {
      bfprintf (&text_out_buffer, "-> error: %s does not refer to a binding", scan_strings[1]);
      return true;
    }

    unbind ((*ptr)->bid);
    unbound_ (ptr);

    return true;
  }
  return false;
}

static bool
destroy_ (void)
{
  if (scan_strings_size >= 1 && strcmp (scan_strings[0], "destroy") == 0) {
    if (scan_strings_size != 2) {
      buffer_file_puts (&text_out_buffer, "-> usage: destroy NAME\n");
      return true;
    }
  
    automaton_t** aptr;
    for (aptr = &automata_head; *aptr != 0; aptr = &(*aptr)->next) {
      if (strcmp ((*aptr)->name, scan_strings[1]) == 0) {
	break;
      }
    }

    if (*aptr == 0) {
      bfprintf (&text_out_buffer, "-> error: %s does not name an automaton", scan_strings[1]);
      return true;
    }

    destroy ((*aptr)->aid);
    destroyed_ (aptr);

    return true;
  }
  return false;
}

/* static void */
/* lookup_usage (void) */
/* { */
/*   buffer_file_puts (&text_out_buffer, "-> usage: NAME = lookup NAME\n"); */
/* } */

/* static bool */
/* lookup_ (void) */
/* { */
/*   if (scan_strings_size >= 1) { */
/*     if (strcmp ("lookup", scan_strings[0]) == 0) { */
/*       lookup_usage (); */
/*       return true; */
/*     } */
/*   } */
/*   if (scan_strings_size >= 3 &&  */
/*       strcmp ("=", scan_strings[1]) == 0 && */
/*       strcmp ("lookup", scan_strings[2]) == 0) { */
/*     if (scan_strings_size == 4) { */
/*       if (find_automaton (scan_strings[0]) != 0) { */
/* 	bfprintf (&text_out_buffer, "-> error: name %s is taken\n", scan_strings[0]); */
/* 	return true; */
/*       } */
      
/*       /\* Perform the lookup. *\/ */
/*       aid_t aid = lookups (scan_strings[3]); */
/*       if (aid == -1) { */
/* 	bfprintf (&text_out_buffer, "-> no automaton registered under %s\n", scan_strings[3]); */
/* 	return true; */
/*       } */
      
/*       automaton_t* a; */
/*       if ((a = find_automaton_aid (aid)) != 0) { */
/* 	bfprintf (&text_out_buffer, "-> error: automaton already exists with name %s\n", a->name); */
/* 	return true; */
/*       } */
      
/*       if (subscribe_destroyed (aid, DESTROYED_NO) != 0) { */
/* 	buffer_file_puts (&text_out_buffer, "-> error: subscribe failed\n"); */
/* 	return true; */
/*       } */

/*       /\* Add to the list of automata we know about. *\/ */
/*       automaton_t* automaton = create_automaton (scan_strings[0], aid, scan_strings[3]); */
      
/*       automaton->next = automata_head; */
/*       automata_head = automaton; */

/*       bfprintf (&text_out_buffer, "-> %s = %d\n", scan_strings[0], aid); */
/*       return true; */
/*     } */
/*     else { */
/*       lookup_usage (); */
/*       return true; */
/*     } */
/*   } */
/*   return false; */
/* } */

static bool
describe_ (void)
{
  if (scan_strings_size >= 1 && strcmp (scan_strings[0], "describe") == 0) {
    if (scan_strings_size != 2) {
      buffer_file_puts (&text_out_buffer, "-> usage: describe NAME\n");
      return true;
    }

    automaton_t* automaton = find_automaton (scan_strings[1]);
    if (automaton == 0) {
      bfprintf (&text_out_buffer, "-> error: name %s not recognized\n", scan_strings[1]);
      return true;
    }

    description_t description;
    if (description_init (&description, automaton->aid) != 0) {
      buffer_file_puts (&text_out_buffer, "-> error: bad description\n");
      return true;
    }

    size_t action_count = description_action_count (&description);
    if (action_count == -1) {
      buffer_file_puts (&text_out_buffer, "-> error: bad description\n");
      description_fini (&description);
      return true;
    }
    
    action_desc_t* actions = malloc (action_count * sizeof (action_desc_t));
    if (description_read_all (&description, actions) != 0) {
      buffer_file_puts (&text_out_buffer, "-> error: bad description\n");
      description_fini (&description);
      free (actions);
      return true;
    }

    bfprintf (&text_out_buffer, "-> aid=%d path=%s\n", automaton->aid, automaton->path);
    
    /* Print the actions. */
    for (size_t action = 0; action != action_count; ++action) {

      buffer_file_puts (&text_out_buffer, "-> ");

      switch (actions[action].type) {
      case LILY_ACTION_INPUT:
	buffer_file_puts (&text_out_buffer, " IN ");
	break;
      case LILY_ACTION_OUTPUT:
	buffer_file_puts (&text_out_buffer, "OUT ");
	break;
      case LILY_ACTION_INTERNAL:
	buffer_file_puts (&text_out_buffer, "INT ");
	break;
      case LILY_ACTION_SYSTEM_INPUT:
	buffer_file_puts (&text_out_buffer, "SYS ");
	break;
      default:
	buffer_file_puts (&text_out_buffer, "??? ");
	break;
      }
      
      switch (actions[action].parameter_mode) {
      case NO_PARAMETER:
	buffer_file_puts (&text_out_buffer, "  ");
	break;
      case PARAMETER:
	buffer_file_puts (&text_out_buffer, "* ");
	break;
      case AUTO_PARAMETER:
	buffer_file_puts (&text_out_buffer, "@ ");
	break;
      default:
	buffer_file_puts (&text_out_buffer, "? ");
	break;
      }
      
      bfprintf (&text_out_buffer, "%d\t%s\t%s\n", actions[action].number, actions[action].name, actions[action].description);
    }
    
    free (actions);
    
    description_fini (&description);

    /* Print the input bindings. */
    for (binding_t* binding = bindings_head; binding != 0; binding = binding->next) {
      if (binding->input_automaton == automaton) {
	bfprintf (&text_out_buffer, "-> %d: (%s, %s, %d) -> (%s, %s, %d)\n", binding->bid, binding->output_automaton->name, binding->output_action_name, binding->output_parameter, binding->input_automaton->name, binding->input_action_name, binding->input_parameter);
      }
    }

    for (binding_t* binding = bindings_head; binding != 0; binding = binding->next) {
      if (binding->output_automaton == automaton) {
	bfprintf (&text_out_buffer, "-> %d: (%s, %s, %d) -> (%s, %s, %d)\n", binding->bid, binding->output_automaton->name, binding->output_action_name, binding->output_parameter, binding->input_automaton->name, binding->input_action_name, binding->input_parameter);
      }
    }
    
    return true;
  }
  return false;
}

static bool
list_ (void)
{
  if (scan_strings_size >= 1) {
    if (strcmp ("list", scan_strings[0]) == 0) {
      if (scan_strings_size == 1) {

	for (automaton_t* automaton = automata_head; automaton != 0; automaton = automaton->next) {
 	  bfprintf (&text_out_buffer, "-> %s\n", automaton->name);
	}
	
	return true;
      }
      else {
	buffer_file_puts (&text_out_buffer, "-> usage: list\n");
	return true;
      }
    }
  }
  return false;
}

static bool
com_ (void)
{
  if (scan_strings_size >= 1 && strcmp (scan_strings[0], "com") == 0) {
    if (scan_strings_size < 3) {
      buffer_file_puts (&text_out_buffer, "-> usage: com ID ARGS...\n");
      return true;
    }

    automaton_t* automaton = find_automaton (scan_strings[1]);
    if (automaton == 0) {
      bfprintf (&text_out_buffer, "-> warning: automaton %s\n", scan_strings[1]);
      return true;
    }

    /* Check if we are already bound. */
    binding_t* b;
    for (b = bindings_head; b != 0; b = b->next) {
      if (b->output_automaton == this_automaton &&
	  b->input_automaton == automaton &&
	  b->output_action == COM_OUT_NO) {
	break;
      }
    }
    if (b == 0) {
      /* Bind to com. */
      single_bind (this_automaton, "com_out", 0, automaton, "com_in", 0);
    }

    for (b = bindings_head; b != 0; b = b->next) {
      if (b->output_automaton == automaton &&
	  b->input_automaton == this_automaton &&
	  b->input_action == COM_IN_NO) {
	break;
      }
    }
    if (b == 0) {
      /* Bind to com. */
      single_bind (automaton, "com_out", 0, this_automaton, "com_in", 0);
    }
	
    /* Add to the COM queue. */
    com_queue_push (automaton->aid, current_line + (scan_strings[2] - scan_string_copy));

    return true;
  }
  return false;
}

static bool
error_ (void)
{
  if (scan_strings_size != 0) {
    /* Catch all. */
    bfprintf (&text_out_buffer, "-> error: unknown command %s\n", scan_strings[0]);
  }
  return true;
}

typedef bool (*dispatch_func_t) (void);

static dispatch_func_t dispatch[] = {
  create_,
  bind_,
  unbind_,
  destroy_,
  /* lookup_, */
  describe_,
  list_,
  com_,
  /* Catch errors. */
  error_,
  0
};

/*
  End Dispatch Section
  ====================
*/

/* static void */
/* readscript_callback (void* data, */
/* 		     bd_t bda, */
/* 		     bd_t bdb) */
/* { */
/*   vfs_error_t error; */
/*   size_t size; */
/*   if (read_vfs_readfile_response (bda, &error, &size) != 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read vfs readfile response: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   if (error != VFS_SUCCESS) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read script: %s", vfs_error_string (error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   const char* str = buffer_map (bdb); */
/*   if (str == 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not map script: %s", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */
  
/*   char* str_copy = malloc (size + 1); */
/*   memcpy (str_copy, str, size); */
/*   /\* Terminate with newline. *\/ */
/*   str_copy[size] = '\n'; */
/*   string_push_front (str_copy, size + 1); */
/*   buffer_unmap (bdb); */
/*   process_hold = false; */
/* } */

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    vfs_init (&vfs, VFS_REQUEST_NO, VFS_RESPONSE_NO);

    bd_t bda = getinita ();
    bd_t bdb = getinitb ();

    if (bda != -1) {
      buffer_file_t de_buffer;
      if (buffer_file_initr (&de_buffer, bda) != 0) {
      	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize de buffer: %s\n", lily_error_string (lily_error));
      	logs (log_buffer);
      	exit (-1);
      }

      de_val_t* root = de_deserialize (&de_buffer);
      if (root == 0) {
      	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not parse de buffer: %s\n", lily_error_string (lily_error));
      	logs (log_buffer);
      	exit (-1);
      }

      snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "finda = %d\n", de_integer_val (de_get (root, "." FINDA), -1));
      logs (log_buffer);

      snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "script = %s\n", de_string_val (de_get (root, "." ARGS "." "script"), "(none)"));
      logs (log_buffer);

      de_val_t* fs = de_get (root, "." FS);
      if (de_type (fs) == DE_ARRAY) {
	for (size_t idx = 0; idx != de_array_size (fs); ++idx) {
	  de_val_t* entry = de_array_at (fs, idx);

	  aid_t from_aid = de_integer_val (de_get (entry, ".from.aid"), -1);
	  inode_t from_inode = de_integer_val (de_get (entry, ".from.inode"), -1);
	  aid_t to_aid = de_integer_val (de_get (entry, ".to.aid"), -1);
	  inode_t to_inode = de_integer_val (de_get (entry, ".to.inode"), -1);
	  if (to_aid != -1 && to_inode != -1) {
	    if (vfs_append (&vfs, from_aid, from_inode, to_aid, to_inode) != 0) {
	      snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "(vfs) could not bind to %d: %s\n", to_aid, lily_error_string (lily_error));
	      logs (log_buffer);
	    }
	  }
	}
      }
    }

    if (bda != -1) {
      if (buffer_destroy (bda) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy init buffer: %s\n", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
    }
    if (bdb != -1) {
      if (buffer_destroy (bdb) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy init buffer: %s\n", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
    }


    /* aid_t aid = getaid (); */

    /* this_automaton = create_automaton ("this", aid, "this"); */

    /* this_automaton->next = automata_head; */
    /* automata_head = this_automaton; */

    /* text_out_bd = buffer_create (0); */
    /* if (text_out_bd == -1) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create text_out buffer: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
    /* if (buffer_file_initw (&text_out_buffer, text_out_bd) != 0) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize text_out buffer: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */

    /* com_out_bd = buffer_create (0); */
    /* if (com_out_bd == -1) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create com_out buffer: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
    /* if (buffer_file_initw (&com_out_buffer, com_out_bd) != 0) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize com_out buffer: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */

    /* vfs_request_bda = buffer_create (0); */
    /* vfs_request_bdb = buffer_create (0); */
    /* if (vfs_request_bda == -1 || */
    /* 	vfs_request_bdb == -1) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create vfs request buffers: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
    /* vfs_request_queue_init (&vfs_request_queue); */
    /* callback_queue_init (&vfs_response_queue); */

    /* /\* /\\* Bind to the vfs. *\\/ *\/ */
    /* /\* aid_t vfs_aid = lookups (VFS_NAME); *\/ */
    /* /\* if (vfs_aid == -1) { *\/ */
    /* /\*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "no vfs"); *\/ */
    /* /\*   logs (log_buffer); *\/ */
    /* /\*   exit (-1); *\/ */
    /* /\* } *\/ */
    
    /* /\* description_t vfs_description; *\/ */
    /* /\* if (description_init (&vfs_description, vfs_aid) != 0) { *\/ */
    /* /\*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not describe vfs: %s", lily_error_string (lily_error)); *\/ */
    /* /\*   logs (log_buffer); *\/ */
    /* /\*   exit (-1); *\/ */
    /* /\* } *\/ */

    /* /\* action_desc_t vfs_request; *\/ */
    /* /\* if (description_read_name (&vfs_description, &vfs_request, VFS_REQUEST_NAME) != 0) { *\/ */
    /* /\*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "vfs does not contain a request action"); *\/ */
    /* /\*   logs (log_buffer); *\/ */
    /* /\*   exit (-1); *\/ */
    /* /\* } *\/ */

    /* /\* action_desc_t vfs_response; *\/ */
    /* /\* if (description_read_name (&vfs_description, &vfs_response, VFS_RESPONSE_NAME) != 0) { *\/ */
    /* /\*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "vfs does not contain a response action"); *\/ */
    /* /\*   logs (log_buffer); *\/ */
    /* /\*   exit (-1); *\/ */
    /* /\* } *\/ */
    
    /* /\* /\\* We bind the response first so they don't get lost. *\\/ *\/ */
    /* /\* if (bind (vfs_aid, vfs_response.number, 0, aid, VFS_RESPONSE_NO, 0) == -1 || *\/ */
    /* /\* 	bind (aid, VFS_REQUEST_NO, 0, vfs_aid, vfs_request.number, 0) == -1) { *\/ */
    /* /\*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not bind to vfs: %s", lily_error_string (lily_error)); *\/ */
    /* /\*   logs (log_buffer); *\/ */
    /* /\*   exit (-1); *\/ */
    /* /\* } *\/ */

    /* /\* description_fini (&vfs_description); *\/ */

    /* if (bda != -1) { */
    /*   /\* Process arguments. *\/ */
    /*   buffer_file_t bf; */
    /*   if (buffer_file_initr (&bf, bda) != 0) { */
    /* 	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize argument buffer: %s", lily_error_string (lily_error)); */
    /* 	logs (log_buffer); */
    /* 	exit (-1); */
    /*   } */

    /*   const size_t size = buffer_file_size (&bf); */
    /*   const char* begin = buffer_file_readp (&bf, size); */
    /*   if (begin == 0) { */
    /* 	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read argument buffer: %s", lily_error_string (lily_error)); */
    /* 	logs (log_buffer); */
    /* 	exit (-1); */
    /*   } */
    /*   const char* end = begin + size; */
    /*   const char* ptr = begin; */

    /*   char* key = 0; */
    /*   char* value = 0; */
    /*   while (ptr != end && kv_parse (&key, &value, &ptr, end) == 0) { */
    /* 	if (key != 0) { */
    /* 	  if (strcmp (key, "script") == 0) { */
    /* 	    if (value != 0) { */
    /* 	      vfs_request_queue_push_readfile (&vfs_request_queue, value); */
    /* 	      process_hold = true; */
    /* 	      callback_queue_push (&vfs_response_queue, readscript_callback, 0); */
    /* 	    } */
    /* 	  } */
    /* 	} */
    /* 	free (key); */
    /* 	free (value); */
    /*   } */
    /* } */
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

BEGIN_INPUT (NO_PARAMETER, TEXT_IN_NO, "text_in", "buffer_file_t", text_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) != 0) {
    finish_input (bda, bdb);
  }

  size_t size = buffer_file_size (&input_buffer);
  const char* str = buffer_file_readp (&input_buffer, size);
  if (str == 0) {
    finish_input (bda, bdb);
  }

  char* str_copy = malloc (size);
  memcpy (str_copy, str, size);

  string_push_back (str_copy, size);

  finish_input (bda, bdb);
}

static bool
process_line_precondition (void)
{
  return !process_hold && !string_empty () && com_queue_empty () && callback_queue_empty (&vfs_response_queue);
}

BEGIN_INTERNAL (NO_PARAMETER, PROCESS_LINE_NO, "", "", process_line, ano_t ano, int param)
{
  initialize ();

  if (process_line_precondition ()) {
    char* current_string;
    size_t current_string_size;
    string_front (&current_string, &current_string_size);

    for (; current_string_idx != current_string_size; ++current_string_idx) {
      const char c = current_string[current_string_idx];
      if (c >= ' ' && c < 127) {
	/* Printable character. */
	line_append (c);
	buffer_file_put (&text_out_buffer, c);
      }
      else {
	/* Control character. */
	switch (c) {
	case '\b':
	  line_back ();
	  buffer_file_put (&text_out_buffer, '\b');
	  buffer_file_put (&text_out_buffer, ' ');
	  buffer_file_put (&text_out_buffer, '\b');
	  break;
	case '\n':
	  /* Terminate. */
	  line_append (0);
	  buffer_file_put (&text_out_buffer, '\n');

	  if (scanner_process (current_line, current_line_size) == 0) {
	    /* Scanning succeeded.  Interpret. */
	    for (size_t idx = 0; dispatch[idx] != 0; ++idx) {
	      if (dispatch[idx] ()) {
		break;
	      }
	    }
	  }

	  /* Reset the line. */
	  line_reset ();

	  ++current_string_idx;

	  /* Done. */
	  finish_internal ();

	  break;
	}
      }
    }

    string_pop ();
    free (current_string);
    current_string_idx = 0;
  }
  finish_internal ();
}

static bool
text_out_precondition (void)
{
  return buffer_file_size (&text_out_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, TEXT_OUT_NO, "text_out", "buffer_file_t", text_out, ano_t ano, int param, size_t bc)
{
  initialize ();

  if (text_out_precondition ()) {
    buffer_file_truncate (&text_out_buffer);
    finish_output (true, text_out_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

BEGIN_SYSTEM_INPUT (DESTROYED_NO, "", "", destroyed, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  
  automaton_t** aptr;
  for (aptr = &automata_head; *aptr != 0; aptr = &(*aptr)->next) {
    if ((*aptr)->aid == aid) {
      break;
    }
  }

  destroyed_ (aptr);

  finish_input (bda, bdb);
}

BEGIN_SYSTEM_INPUT (UNBOUND_NO, "", "", unbound, ano_t ano, bid_t bid, bd_t bda, bd_t bdb)
{
  initialize ();

  binding_t** ptr;
  for (ptr = &bindings_head; *ptr != 0; ptr = &(*ptr)->next) {
    if ((*ptr)->bid == bid) {
      break;
    }
  }

  unbound_ (ptr);

  finish_input (bda, bdb);
}

/* vfs_request
   -------------
   Send a request to the vfs.

   Pre:  vfs_request_queue is not empty
   Post: vfs_request_queue has one item removed
 */
static bool
vfs_request_precondition (void)
{
  return !vfs_request_queue_empty (&vfs_request_queue);
}

BEGIN_OUTPUT (NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, ano_t ano, int param, size_t bc)
{
  initialize ();

  if (vfs_request_precondition ()) {
    if (vfs_request_queue_pop_to_buffer (&vfs_request_queue, vfs_request_bda, vfs_request_bdb) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could pop vfs request: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    finish_output (true, vfs_request_bda, vfs_request_bdb);
  }
  else {
    finish_output (false, -1, -1);
  }
}

/* vfs_response
   --------------
   Invoke the callback for a response from the vfs.

   Post: the callback queue has one item removed
 */
BEGIN_INPUT (NO_PARAMETER, VFS_RESPONSE_NO, "", "", vfs_response, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (callback_queue_empty (&vfs_response_queue)) {
    finish_input (bda, bdb);
  }

  const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
  callback_t callback = callback_queue_item_callback (item);
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&vfs_response_queue);

  callback (data, bda, bdb);

  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, COM_OUT_NO, "com_out", "", com_out, ano_t ano, aid_t aid, size_t bc)
{
  initialize ();

  if (!com_queue_empty () && aid == com_queue_front ()) {
    buffer_file_truncate (&com_out_buffer);
    buffer_file_puts (&com_out_buffer, com_head->str);
    com_queue_pop ();
    finish_output (true, com_out_bd, -1);
  }
  finish_output (false, -1, -1);
}

BEGIN_INPUT (AUTO_PARAMETER, COM_IN_NO, "com_in", "", com_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t bf;
  if (buffer_file_initr (&bf, bda) != 0) {
    finish_input (bda, bdb);
  }

  size_t size = buffer_file_size (&bf);
  if (size == 0) {
    finish_input (bda, bdb);
  }
  const char* begin = buffer_file_readp (&bf, size);
  if (begin == 0) {
    finish_input (bda, bdb);
  }

  buffer_file_write (&text_out_buffer, begin, size);

  finish_input (bda, bdb);
}

void
do_schedule (void)
{
  if (process_line_precondition ()) {
    schedule (PROCESS_LINE_NO, 0);
  }
  if (text_out_precondition ()) {
    schedule (TEXT_OUT_NO, 0);
  }
  if (vfs_request_precondition ()) {
    schedule (VFS_REQUEST_NO, 0);
  }
  if (!com_queue_empty ()) {
    schedule (COM_OUT_NO, com_queue_front ());
  }
}
