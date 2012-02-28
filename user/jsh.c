#include <automaton.h>
#include <io.h>
#include <string.h>
#include <buffer_queue.h>
#include <callback_queue.h>
#include <fifo_scheduler.h>
#include <description.h>
#include <dymem.h>
#include "vfs_msg.h"
#include "argv.h"

#define DESTROY_BUFFERS_NO 1
#define VFS_RESPONSE_NO 2
#define VFS_REQUEST_NO 3
#define LOAD_TEXT_NO 4
#define PROCESS_TEXT_NO 5

#define VFS_NAME "vfs"

/* Initialization flag. */
static bool initialized = false;

/* Queue of buffers that need to be destroyed. */
static buffer_queue_t destroy_queue;

/* The aid of the vfs. */
static aid_t vfs_aid = -1;

/* Messages to the vfs. */
static buffer_queue_t vfs_request_queue;

/* Callbacks to execute from the vfs. */
static callback_queue_t vfs_response_queue;

/* Buffers to interpret. */
static buffer_queue_t interpret_queue;

/* Text to interpret. */
static bd_t interpret_bd = -1;
static const char* interpret_string;
static size_t interpret_string_size;
static size_t interpret_string_idx;

/* Flag indicating if we are asynchronously evaluating. */
static bool evaluating = false;

static void
ssyslog (const char* msg)
{
  syslog (msg, strlen (msg));
}

/* Parsing functions. */
typedef enum {
  CREATE,
  CREATEP,
  BIND,
  STRING,
  ASSIGN,
} token_type_t;

typedef struct token_list_item token_list_item_t;
struct token_list_item {
  token_type_t type;
  char* string;
  size_t size;
  token_list_item_t* next;
};

static token_list_item_t* token_list_head = 0;
static token_list_item_t** token_list_tail = &token_list_head;
static token_list_item_t* current_token;

static void
push_token (token_type_t type,
	    char* string,
	    size_t size)
{
  /* Override the type if a keyword. */
  if (type == STRING) {
    if (strncmp ("create", string, size) == 0) {
      type = CREATE;
    }
    else if (strncmp ("createp", string, size) == 0) {
      type = CREATEP;
    }
    else if (strncmp ("bind", string, size) == 0) {
      type = BIND;
    }
  }

  token_list_item_t* item = malloc (sizeof (token_list_item_t));
  memset (item, 0, sizeof (token_list_item_t));
  item->type = type;
  item->string = string;
  item->size = size;
  *token_list_tail = item;
  token_list_tail = &item->next;
}

static void
clean_tokens (void)
{
  /* Cleanup the list of token. */
  while (token_list_head != 0) {
    token_list_item_t* item = token_list_head;
    token_list_head = item->next;
    free (item->string);
    free (item);
  }
  token_list_tail = &token_list_head;
}

static token_list_item_t*
accept (token_type_t type)
{
  if (current_token == 0) {
    return 0;
  }

  if (current_token->type == type) {
    token_list_item_t* item = current_token;
    current_token = current_token->next;
    return item;
  }
  else {
    return 0;
  }
}

static token_list_item_t*
expect (token_type_t type,
	const char* error)
{
  if (current_token != 0 && current_token->type == type) {
    token_list_item_t* item = current_token;
    current_token = current_token->next;
    return item;
  }
  else {
    /* TODO */
    ssyslog (error);
    return 0;
  }
}

static void
create_ (void)
{
  /* TODO */
  ssyslog ("CREATE\n");
}

static void
createp_callback (void* data,
		  bd_t bda,
		  bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) == -1) {
    ssyslog ("jsh: error: vfs provide bad readfile response\n");
    exit ();
  }

  if (error == VFS_SUCCESS) {
    /* TODO:  Create an argv for the automaton. */
    aid_t aid = create (bdb, size, -1, -1, true);
    if (aid == -1) {
      /* TODO */
      ssyslog ("createp create fail\n");
    }
    /* TODO */
    ssyslog ("assign createp result to variable\n");
  }
  else {
    /* TODO */
    ssyslog ("createp:  couldn't read file\n");
  }

  /* No longer evaluating. */
  evaluating = false;
}

static void
createp (void)
{
  token_list_item_t* f;
  
  if ((f = expect (STRING, "Expected a file\n")) != 0) {
    /* Request the text of the automaton. */
    bd_t bd = write_vfs_readfile_request (f->string);
    if (bd == -1) {
      ssyslog ("jsh: error: Couldn't create readfile request\n");
      exit ();
    }
    buffer_queue_push (&vfs_request_queue, 0, bd, 0, -1, 0);
    callback_queue_push (&vfs_response_queue, createp_callback, 0);
    /* Set the flag so we stop trying to evaluate. */
    evaluating = true;
  }
}

static void
bind_ (void)
{
  /* TODO */
  ssyslog ("BIND\n");
}

static void
statement (void)
{
  token_list_item_t* t;

  if ((t = accept (STRING)) != 0) {
    if (expect (ASSIGN, "Expected =\n") == 0) {
      return;
    }
    /* TODO: Set up machinery to assign a variable. */
    ssyslog ("ASSIGN ");
    syslog (t->string, t->size);
    ssyslog (" ");
  }

  if ((t = accept (CREATE)) != 0) {
    create_ ();
  }
  else if ((t = accept (CREATEP)) != 0) {
    createp ();
  }
  else if ((t = accept (BIND)) != 0) {
    bind_ ();
  }
  else {
    /* TODO: Syntax error. */
    ssyslog ("syntax error\n");
  }
}

static void
evaluate (void)
{
  if (token_list_head != 0) {
    /* Try to evaluate a statement. */
    current_token = token_list_head;
    statement ();
    clean_tokens ();
  }
}

/* Scanning functions. */
typedef enum {
  SCAN_START,
  SCAN_STRING,
  SCAN_COMMENT,
} scan_state_t;

static scan_state_t scan_state = SCAN_START;
static char* scan_string = 0;
static size_t scan_string_size = 0;
static size_t scan_string_capacity = 0;

static void
scan_string_append (char c)
{
  if (scan_string_size == scan_string_capacity) {
    scan_string_capacity = 2 * scan_string_size;
    scan_string = realloc (scan_string, scan_string_capacity);
  }
  scan_string[scan_string_size++] = c;
}

static void
scan_string_reset (void)
{
  /* Potentially make this the average size of a string. */
  scan_string_capacity = 1;
  scan_string = malloc (scan_string_capacity);
  scan_string_size = 0;
}

/* Language

   string - [a-zA-Z0-9_/]+

   assign symbol - =

   whitespace - [ \t]

   comment - #[^\n]*

   evaluation symbol - [;\n]
 */
static void
put (char c)
{
  switch (scan_state) {
  case SCAN_START:
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9') ||
	c == '_' ||
	c == '/') {
      scan_string_append (c);
      scan_state = SCAN_STRING;
      break;
    }

    switch (c) {
    case '=':
      push_token (ASSIGN, 0, 0);
      break;
    case '#':
      scan_state = SCAN_COMMENT;
      break;
    case ';':
    case '\n':
      evaluate ();
      break;
    case ' ':
    case '\t':
      /* Each whitespace. */
      break;
    default:
      ssyslog ("ignoring: ");
      syslog (&c, 1);
      ssyslog ("\n");
      break;
    }
    break;

  case SCAN_STRING:
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9') ||
	c == '_' ||
	c == '/') {
      /* Continue. */
      scan_string_append (c);
      break;
    }

    /* Terminate with null. */
    scan_string_append (0);
    push_token (STRING, scan_string, scan_string_size);
    scan_string_reset ();
    
    switch (c) {
    case '=':
      push_token (ASSIGN, 0, 0);
      scan_state = SCAN_START;
      break;
    case '#':
      scan_state = SCAN_COMMENT;
      break;
    case ';':
    case '\n':
      evaluate ();
      scan_state = SCAN_START;
      break;
    case ' ':
    case '\t':
      scan_state = SCAN_START;
      break;
    default:
      ssyslog ("ignoring: ");
      syslog (&c, 1);
      ssyslog ("\n");
      break;
    }
    break;

  case SCAN_COMMENT:
    switch (c) {
    case '\n':
      evaluate ();
      scan_state = SCAN_START;
      break;
    }
    break;
  }
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    buffer_queue_init (&destroy_queue);
    buffer_queue_init (&vfs_request_queue);
    callback_queue_init (&vfs_response_queue);
    buffer_queue_init (&interpret_queue);
    scan_string_reset ();
  }
}

static void
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb);

static void
readscript_callback (void* data,
		   bd_t bda,
		   bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) == -1) {
    ssyslog ("jsh: error: vfs provide bad readfile response\n");
    exit ();
  }

  if (error != VFS_SUCCESS) {
    ssyslog ("jsh: error: readfile failed \n");
    exit ();
  }

  /* Put the file on the interpret queue. */
  buffer_queue_push (&interpret_queue, 0, -1, 0, buffer_copy (bdb), size);
}

/* init
   ----
   The script to execute comes in on bda.

   Post: ???
 */
BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t aid, bd_t bda, bd_t bdb)
{
  ssyslog ("jsh: init\n");
  initialize ();

  /* Bind to the vfs. */
  vfs_aid = lookup (VFS_NAME, strlen (VFS_NAME));
  if (vfs_aid == -1) {
    ssyslog ("jsh: error: no vfs\n");
    exit ();
  }

  description_t vfs_description;
  if (description_init (&vfs_description, vfs_aid) == -1) {
    ssyslog ("jsh: error: couldn't describe vfs\n");
    exit ();
  }
  const ano_t vfs_request = description_name_to_number (&vfs_description, VFS_REQUEST_NAME);
  const ano_t vfs_response = description_name_to_number (&vfs_description, VFS_RESPONSE_NAME);
  description_fini (&vfs_description);

  if (vfs_request == NO_ACTION ||
      vfs_response == NO_ACTION) {
    ssyslog ("jsh: error: the vfs does not appear to be a vfs\n");
    exit ();
  }

  /* We bind the response first so they don't get lost. */
  if (bind (vfs_aid, vfs_response, 0, aid, VFS_RESPONSE_NO, 0) == -1 ||
      bind (aid, VFS_REQUEST_NO, 0, vfs_aid, vfs_request, 0) == -1) {
    ssyslog ("jsh: error: Couldn't bind to vfs\n");
    exit ();
  }

  argv_t argv;
  size_t argc;
  if (argv_initr (&argv, bda, bdb, &argc) != -1) {
    if (argc >= 2) {
      /* Request the script. */
      const char* filename = argv_arg (&argv, 1);

      bd_t bd = write_vfs_readfile_request (filename);
      if (bd == -1) {
	ssyslog ("jsh: error: Couldn't create readfile request\n");
	exit ();
      }
      buffer_queue_push (&vfs_request_queue, 0, bd, 0, -1, 0);
      callback_queue_push (&vfs_response_queue, readscript_callback, 0);
    }
  }

  end_action (false, bda, bdb);
}

/* destroy_buffers
   ---------------
   Destroys all of the buffers in destroy_queue.
   This is useful for output actions that need to destroy the buffer *after* the output has fired.
   To schedule a buffer for destruction, just add it to destroy_queue.

   Pre:  Destroy queue is not empty.
   Post: Destroy queue is empty.
 */
static bool
destroy_buffers_precondition (void)
{
  return !buffer_queue_empty (&destroy_queue);
}

BEGIN_INTERNAL (NO_PARAMETER, DESTROY_BUFFERS_NO, "", "", destroy_buffers, int param)
{
  initialize ();
  scheduler_remove (DESTROY_BUFFERS_NO, param);

  if (destroy_buffers_precondition ()) {
    while (!buffer_queue_empty (&destroy_queue)) {
      bd_t bd;
      const buffer_queue_item_t* item = buffer_queue_front (&destroy_queue);
      bd = buffer_queue_item_bda (item);
      if (bd != -1) {
	buffer_destroy (bd);
      }
      bd = buffer_queue_item_bdb (item);
      if (bd != -1) {
	buffer_destroy (bd);
      }

      buffer_queue_pop (&destroy_queue);
    }
  }

  end_action (false, -1, -1);
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
  return !buffer_queue_empty (&vfs_request_queue);
}

BEGIN_OUTPUT (NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, int param)
{
  initialize ();
  scheduler_remove (VFS_REQUEST_NO, param);

  if (vfs_request_precondition ()) {
    ssyslog ("jsh: vfs_request\n");
    const buffer_queue_item_t* item = buffer_queue_front (&vfs_request_queue);
    bd_t bda = buffer_queue_item_bda (item);
    bd_t bdb = buffer_queue_item_bdb (item);
    buffer_queue_pop (&vfs_request_queue);

    end_action (true, bda, bdb);
  }
  else {
    end_action (false, -1, -1);
  }
}

/* vfs_response
   --------------
   Invoke the callback for a response from the vfs.

   Post: the callback queue has one item removed
 */
BEGIN_INPUT (NO_PARAMETER, VFS_RESPONSE_NO, "", "", vfs_response, int param, bd_t bda, bd_t bdb)
{
  ssyslog ("jsh: vfs_response\n");
  initialize ();

  if (callback_queue_empty (&vfs_response_queue)) {
    ssyslog ("jsh: error: vfs produced spurious response\n");
    exit ();
  }

  const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
  callback_t callback = callback_queue_item_callback (item);
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&vfs_response_queue);

  callback (data, bda, bdb);

  end_action (false, bda, bdb);
}

/* load_text
   ------------------------
   Remove the first item on the interpret queue and make it available for processing.

   Pre:  interpret_bd == -1 && interpret queue size = N which is not 0
   Post: interpret_bd != -1 && interpret queue size = N - 1
 */
static bool
load_text_precondition (void)
{
  return interpret_bd == -1 && !buffer_queue_empty (&interpret_queue);
}

BEGIN_INTERNAL (NO_PARAMETER, LOAD_TEXT_NO, "", "", load_text, int param)
{
  initialize ();
  scheduler_remove (LOAD_TEXT_NO, param);

  if (load_text_precondition ()) {
    ssyslog ("jsh: load_text\n");
    const buffer_queue_item_t* item = buffer_queue_front (&interpret_queue);
    bd_t bda = buffer_queue_item_bda (item);
    bd_t bdb = buffer_queue_item_bdb (item);
    size_t sizeb = buffer_queue_item_sizeb (item);
    buffer_queue_pop (&interpret_queue);

    if (bdb == -1) {
      end_action (false, bda, -1);
    }
    
    buffer_file_t bf;
    if (buffer_file_open (&bf, bdb, false) == -1) {
      end_action (false, bda, bdb);
    }

    const char* s = buffer_file_readp (&bf, sizeb);
    if (s == 0) {
      end_action (false, bda, bdb);
    }

    interpret_bd = bdb;
    interpret_string = s;
    interpret_string_size = sizeb;
    interpret_string_idx = 0;
    end_action (false, bda, -1);
  }

  end_action (false, -1, -1);
}

/* process_text
   ------------------------
   Process the text to be interpretted.

   Pre:  interpret_bd != -1 && !evaluating
   Post: interpret_bd == -1 || evaluating
 */
static bool
process_text_precondition (void)
{
  return interpret_bd != -1 && !evaluating;
}

BEGIN_INTERNAL (NO_PARAMETER, PROCESS_TEXT_NO, "", "", process_text, int param)
{
  initialize ();
  scheduler_remove (PROCESS_TEXT_NO, param);

  if (process_text_precondition ()) {
    ssyslog ("jsh: process_text\n");

    while (!evaluating && interpret_string_idx != interpret_string_size) {
      put (interpret_string[interpret_string_idx++]);
    }

    if (interpret_string_idx == interpret_string_size) {
      /* Processed the complete buffer. */
      buffer_destroy (interpret_bd);
      interpret_bd = -1;
    }
  }

  end_action (false, -1, -1);
}

/* end_action is a helper function for terminating actions.
   If the buffer is not -1, it schedules it to be destroyed.
   end_action schedules local actions and calls scheduler_finish to finish the action.
*/
static void
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb)
{
  if (bda != -1 || bdb != -1) {
    buffer_queue_push (&destroy_queue, 0, bda, 0, bdb, 0);
  }

  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }
  if (vfs_request_precondition ()) {
    scheduler_add (VFS_REQUEST_NO, 0);
  }
  if (load_text_precondition ()) {
    scheduler_add (LOAD_TEXT_NO, 0);
  }
  if (process_text_precondition ()) {
    scheduler_add (PROCESS_TEXT_NO, 0);
  }

  scheduler_finish (output_fired, bda, bdb);
}
