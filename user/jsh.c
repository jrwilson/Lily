#include <automaton.h>
#include <string.h>
#include <buffer_queue.h>
#include <callback_queue.h>
#include <fifo_scheduler.h>
#include <description.h>
#include <dymem.h>
#include "vfs_msg.h"
#include "argv.h"

/* Messages to the vfs. */
static bd_t vfs_request_bda = -1;
static bd_t vfs_request_bdb = -1;
static vfs_request_queue_t vfs_request_queue;

/* Callbacks to execute from the vfs. */
static callback_queue_t vfs_response_queue;

/*
  Begin Model Section
  ===================
*/

typedef struct automaton automaton_t;
struct automaton {
  aid_t aid;
  char* name;
  size_t name_size;
  automaton_t* next;
};

static automaton_t* automaton_head = 0;

static void
create_automaton (aid_t aid,
		  const char* name,
		  size_t size)
{
  automaton_t* a = malloc (sizeof (automaton_t));
  memset (a, 0, sizeof (automaton_t));
  a->aid = aid;
  a->name = malloc (size);
  memcpy (a->name, name, size);
  a->name_size = size;
  a->next = automaton_head;
  automaton_head = a;
}

static void
destroy_automaton (automaton_t* b)
{
  automaton_t** ptr;
  for (ptr = &automaton_head; *ptr != 0; ptr = &(*ptr)->next) {
    if ((*ptr) == b) {
      *ptr = b->next;
      free (b->name);
      free (b);
      return;
    }
  }
}

static automaton_t*
find_automaton (const char* name,
		size_t size)
{
  automaton_t* a;
  for (a = automaton_head; a != 0; a = a->next) {
    if (a->name_size == size && memcmp (a->name, name, size) == 0) {
      break;
    }
  }
  
  return a;
}

typedef struct binding binding_t;
struct binding {
  bid_t bid;
  char* name;
  size_t name_size;
  binding_t* next;
};

static binding_t* binding_head = 0;

static void
create_binding (bid_t bid,
		const char* name,
		size_t size)
{
  binding_t* a = malloc (sizeof (binding_t));
  memset (a, 0, sizeof (binding_t));
  a->bid = bid;
  a->name = malloc (size);
  memcpy (a->name, name, size);
  a->name_size = size;
  a->next = binding_head;
  binding_head = a;
}

static void
destroy_binding (binding_t* b)
{
  binding_t** ptr;
  for (ptr = &binding_head; *ptr != 0; ptr = &(*ptr)->next) {
    if ((*ptr) == b) {
      *ptr = b->next;
      free (b->name);
      free (b);
      return;
    }
  }
}

static binding_t*
find_binding (const char* name,
	      size_t size)
{
  binding_t* a;
  for (a = binding_head; a != 0; a = a->next) {
    if (a->name_size == size && memcmp (a->name, name, size) == 0) {
      break;
    }
  }
  
  return a;
}

typedef struct start_queue_item start_queue_item_t;
struct start_queue_item {
  aid_t aid;
  start_queue_item_t* next;
};

static start_queue_item_t* start_queue_head;
static start_queue_item_t** start_queue_tail = &start_queue_head;

static void
start_queue_push (aid_t aid)
{
  start_queue_item_t* item = malloc (sizeof (start_queue_item_t));
  memset (item, 0, sizeof (start_queue_item_t));
  item->aid = aid;
  *start_queue_tail = item;
  start_queue_tail = &item->next;
}

static start_queue_item_t**
start_queue_find (aid_t aid)
{
  start_queue_item_t** item;
  for (item = &start_queue_head; *item != 0; item = &(*item)->next) {
    if ((*item)->aid == aid) {
      break;
    }
  }

  return item;
}

static void
start_queue_erase (start_queue_item_t** item)
{
  start_queue_item_t* i = *item;

  *item = i->next;
  if (start_queue_head == 0) {
    start_queue_tail = &start_queue_head;
  }

  free (item);
}

static bool
start_queue_empty (void)
{
  return start_queue_head == 0;
}

static aid_t
start_queue_front (void)
{
  return start_queue_head->aid;
}

/*
  End Model Section
  ========================
*/

/*
  Begin Asynchronous Create Section
  =================================
*/

/*
  Flag indicating that we are asynchronously evaluating.
  Usually, this means we are waiting on I/O.
*/
static bool evaluating = false;

typedef struct {
  char* name;
  size_t name_size;
  bd_t bda;
  bd_t bdb;
  argv_t argv;
  bool retain_privilege;
  char* registry_name;
  size_t registry_name_size;
} create_context_t;

static create_context_t*
create_create_context (const char* name,
		       size_t name_size,
		       bool retain_privilege,
		       const char* registry_name,
		       size_t registry_name_size)
{
  create_context_t* cc = malloc (sizeof (create_context_t));
  memset (cc, 0, sizeof (create_context_t));
  cc->name = malloc (name_size);
  memcpy (cc->name, name, name_size);
  cc->name_size = name_size;
  if (argv_initw (&cc->argv, &cc->bda, &cc->bdb) != 0) {
    syslog ("jsh: error: could create argv");
    exit ();
  }
  cc->retain_privilege = retain_privilege;
  cc->registry_name = malloc (registry_name_size);
  memcpy (cc->registry_name, registry_name, registry_name_size);
  cc->registry_name_size = registry_name_size;
  return cc;
}

static void
destroy_create_context (create_context_t* cc)
{
  if (cc->bda != -1) {
    buffer_destroy (cc->bda);
  }
  if (cc->bdb != -1) {
    buffer_destroy (cc->bdb);
  }
  free (cc->name);
  free (cc->registry_name);
  free (cc);
}

static void
create_callback (void* data,
		 bd_t bda,
		 bd_t bdb)
{
  create_context_t* cc = data;
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) != 0) {
    syslog ("jsh: error: vfs provide bad readfile response");
    exit ();
  }

  if (error == VFS_SUCCESS) {
    aid_t aid = create (bdb, size, cc->bda, cc->bdb, cc->registry_name, cc->registry_name_size, cc->retain_privilege);
    if (aid != -1) {
      syslog ("TODO:  Subscribe to created automaton");
      /* Assign the result to a variable. */
      create_automaton (aid, cc->name, cc->name_size);
    }
    else {
      syslog ("TODO:  create create failed");
    }
  }
  else {
    syslog ("TODO:  create:  couldn't read file");
  }

  destroy_create_context (cc);

  /* No longer evaluating. */
  evaluating = false;
}

/*
  End Asynchronous Create Section
  ===============================
*/

/*
  Begin Parser/Evaluator Section
  ==============================
*/

typedef enum {
  AUTOMATON_VAR,
  BINDING_VAR,
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
  if (current_token != 0 && current_token->type == type) {
    token_list_item_t* item = current_token;
    current_token = current_token->next;
    return item;
  }
  else {
    return 0;
  }
}

static void
create_ (token_list_item_t* var)
{
  bool retain_privilege = false;
  const char* registry_name = 0;
  size_t registry_name_size = 0;
  token_list_item_t* filename;
  
  if (find_automaton (var->string, var->size) != 0) {
    syslog ("TODO:  Automaton variable already assigned");
    return;
  }

  while (current_token != 0 &&
	 current_token->type == STRING) {
    if (strcmp (current_token->string, "-p") == 0) {
      retain_privilege = true;
      accept (STRING);
    }
    else if (strcmp (current_token->string, "-n") == 0) {
      accept (STRING);
      token_list_item_t* reg_name;
      if ((reg_name = accept (STRING)) == 0) {
	syslog ("TODO:  Expected a name");
	return;
      }
      registry_name = reg_name->string;
      registry_name_size = reg_name->size;
    }
    else {
      break;
    }
  }

  if ((filename = accept (STRING)) == 0) {
    syslog ("TODO:  Expected a filename or argument");
    return;
  }

  token_list_item_t* string;
  for (string = current_token; string != 0; string = string->next) {
    if (string->type != STRING) {
      syslog ("TODO:  Expected a string");
      return;
    }
  }

  /* Create context for the create callback. */
  create_context_t* cc = create_create_context (var->string, var->size, retain_privilege, registry_name, registry_name_size);
  
  while ((string = accept (STRING)) != 0) {
    /* Add the string to argv. */
    argv_append (&cc->argv, string->string, string->size);
  }
  
  /* Request the text of the automaton. */
  vfs_request_queue_push_readfile (&vfs_request_queue, filename->string);
  callback_queue_push (&vfs_response_queue, create_callback, cc);
  /* Set the flag so we stop trying to evaluate. */
  evaluating = true;
}

static void
bind_ (token_list_item_t* var)
{
  if (var != 0 && find_binding (var->string, var->size) != 0) {
    syslog ("TODO:  Binding variable already assigned");
    return;
  }

  /* Get the argument tokens. */
  token_list_item_t* output_automaton_token;
  token_list_item_t* output_action_token;
  token_list_item_t* input_automaton_token;
  token_list_item_t* input_action_token;
  if ((output_automaton_token = accept (AUTOMATON_VAR)) == 0) {
    syslog ("TODO:  Expected an automaton variable");
    return;
  }

  if ((output_action_token = accept (STRING)) == 0) {
    syslog ("TODO:  Expected a string");
    return;
  }

  if ((input_automaton_token = accept (AUTOMATON_VAR)) == 0) {
    syslog ("TODO:  Expected an automaton variable");
    return;
  }

  if ((input_action_token = accept (STRING)) == 0) {
    syslog ("TODO:  Expected a string");
    return;
  }

  if (current_token != 0) {
    syslog ("TODO:  Expected no more tokens");
    return;
  }

  /* Look up the variables for the output and input automaton. */
  automaton_t* output_automaton = find_automaton (output_automaton_token->string, output_automaton_token->size);
  if (output_automaton == 0) {
    syslog ("TODO:  Output automaton is not defined");
    return;
  }
  automaton_t* input_automaton = find_automaton (input_automaton_token->string, input_automaton_token->size);
  if (input_automaton == 0) {
    syslog ("TODO:  Input automaton is not defined");
    return;
  }

  /* Describe the output and input automaton. */
  description_t output_description;
  description_t input_description;
  if (description_init (&output_description, output_automaton->aid) != 0) {
    syslog ("TODO:  Could not describe output");
    return;
  }
  if (description_init (&input_description, input_automaton->aid) != 0) {
    syslog ("TODO:  Could not describe input");
    return;
  }

  /* Look up the actions. */
  const ano_t output_action = description_name_to_number (&output_description, output_action_token->string, output_action_token->size);
  const ano_t input_action = description_name_to_number (&input_description, input_action_token->string, input_action_token->size);

  description_fini (&output_description);
  description_fini (&input_description);

  if (output_action == NO_ACTION) {
    syslog ("TODO:  Output action does not exist");
    return;
  }

  if (input_action == NO_ACTION) {
    syslog ("TODO:  Input action does not exist");
    return;
  }

  bid_t bid = bind (output_automaton->aid, output_action, 0, input_automaton->aid, input_action, 0);
  if (bid == -1) {
    syslog ("TODO:  Bind failed");
    return;
  }

  if (var == 0) {
    syslog ("TODO:  Create binding");
    /* char buffer[sizeof (bid_t) * 8]; */
    /* size_t sz = snprintf (buffer, sizeof (bid_t) * 8, "&b%d", bid); */
    /* create_binding (bid, buffer, sz + 1); */
  }
  else {
    create_binding (bid, var->string, var->size);
  }
  
  syslog ("TODO:  Subscribe to binding??");
}

static void
unbind_ ()
{
  /* Get the argument tokens. */
  token_list_item_t* binding_token;
  if ((binding_token = accept (BINDING_VAR)) == 0) {
    syslog ("TODO:  Expected a binding variable");
    return;
  }

  if (current_token != 0) {
    syslog ("TODO:  Expected no more tokens");
    return;
  }

  /* Look up the binding variable. */
  binding_t* binding = find_binding (binding_token->string, binding_token->size);
  if (binding == 0) {
    syslog ("TODO:  Binding is not defined");
    return;
  }

  syslog ("TODO:  Unsubscribe from binding??");

  if (unbind (binding->bid) != 0) {
    syslog ("TODO:  unbind failed");
  }

  destroy_binding (binding);
}

static void
destroy_ ()
{
  /* Get the argument tokens. */
  token_list_item_t* automaton_token;
  if ((automaton_token = accept (AUTOMATON_VAR)) == 0) {
    syslog ("TODO:  Expected an automaton variable");
    return;
  }

  if (current_token != 0) {
    syslog ("TODO:  Expected no more tokens");
    return;
  }

  /* Look up the automaton variable. */
  automaton_t* automaton = find_automaton (automaton_token->string, automaton_token->size);
  if (automaton == 0) {
    syslog ("TODO:  Automaton is not defined");
    return;
  }

  syslog ("TODO:  Unsubscribe from automaton??");

  if (destroy (automaton->aid) != 0) {
    syslog ("TODO:  destroy failed");
  }

  destroy_automaton (automaton);
}

static void
start_ (void)
{
  /* Get the argument token. */
  token_list_item_t* automaton_token;
  if ((automaton_token = accept (AUTOMATON_VAR)) == 0) {
    syslog ("TODO:  Expected an automaton variable");
    return;
  }

  if (current_token != 0) {
    syslog ("TODO:  Expected no more tokens");
    return;
  }

  /* Look up the variables for the automaton. */
  automaton_t* automaton = find_automaton (automaton_token->string, automaton_token->size);
  if (automaton == 0) {
    syslog ("TODO:  Automaton is not defined");
    return;
  }

  /* TODO:  If we are not bound to it, then we should probably warn them. */

  start_queue_push (automaton->aid);
}

static void
automaton_assignment (token_list_item_t* var)
{
  if (accept (ASSIGN) == 0) {
    syslog ("TODO:  Expected =");
    return;
  }
  
  token_list_item_t* t;
  if ((t = accept (STRING)) != 0 && strncmp ("create", t->string, t->size) == 0) {
    create_ (var);
  }
  else {
    syslog ("TODO:  syntax error");
  }
}

static void
binding_assignment (token_list_item_t* var)
{
  if (accept (ASSIGN) == 0) {
    syslog ("TODO:  Expected =");
    return;
  }
  
  token_list_item_t* t;
  if ((t = accept (STRING)) != 0 && strncmp ("bind", t->string, t->size) == 0) {
    bind_ (var);
  }
  else {
    syslog ("TODO:  syntax error");
  }
}

static void
statement (void)
{
  token_list_item_t* t;

  if ((t = accept (AUTOMATON_VAR)) != 0) {
    automaton_assignment (t);
    return;
  }
  else if ((t = accept (BINDING_VAR)) != 0) {
    binding_assignment (t);
    return;
  }
  else if ((t = accept (STRING)) != 0) {
    if (strncmp ("bind", t->string, t->size) == 0) {
      bind_ (0);
      return;
    }
    else if (strncmp ("unbind", t->string, t->size) == 0) {
      unbind_ ();
      return;
    }
    else if (strncmp ("destroy", t->string, t->size) == 0) {
      destroy_ ();
      return;
    }
    else if (strncmp ("start", t->string, t->size) == 0) {
      start_ ();
      return;
    }
  }

  syslog ("TODO:  syntax error");
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

/*
  End Parser/Evaluator Section
  ============================
*/

/*
  Begin Scanner Section
  =====================
*/

typedef enum {
  SCAN_START,
  SCAN_AUTOMATON_VAR,
  SCAN_BINDING_VAR,
  SCAN_STRING,
  SCAN_COMMENT,
  SCAN_ERROR,
} scan_state_t;

static scan_state_t scan_state = SCAN_START;
static char* scan_string = 0;
static size_t scan_string_size = 0;
static size_t scan_string_capacity = 0;

static void
scan_string_init (void)
{
  /* Potentially make this the average size of a string. */
  scan_string_capacity = 1;
  scan_string = malloc (scan_string_capacity);
  scan_string_size = 0;
}

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
scan_string_steal (char** string,
		   size_t* size)
{
  *string = scan_string;
  *size = scan_string_size;
  /* Potentially make this the average size of a string. */
  scan_string_capacity = 1;
  scan_string = malloc (scan_string_capacity);
  scan_string_size = 0;
}

/* Tokens

   string - [a-zA-Z0-9_/-]+

   assign symbol - =

   whitespace - [ \t]

   comment - #[^\n]*

   evaluation symbol - [;\n]

   automaton variable - @[a-zA-Z0-9_]+

   binding_variable - &[a-zA-Z0-9_]+
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
	c == '/' ||
	c == '-') {
      scan_string_append (c);
      scan_state = SCAN_STRING;
      break;
    }

    switch (c) {
    case '@':
      scan_string_append (c);
      scan_state = SCAN_AUTOMATON_VAR;
      break;
    case '&':
      scan_string_append (c);
      scan_state = SCAN_BINDING_VAR;
      break;
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
      scan_string_append (c);
      scan_state = SCAN_ERROR;
      break;
    }
    break;

  case SCAN_STRING:
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9') ||
	c == '_' ||
	c == '/' ||
	c == '-') {
      /* Continue. */
      scan_string_append (c);
      break;
    }

    {
      /* Terminate with null. */
      scan_string_append (0);
      char* string;
      size_t size;
      scan_string_steal (&string, &size);
      push_token (STRING, string, size);
    }
    
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
      scan_state = SCAN_ERROR;
      break;
    }
    break;

  case SCAN_AUTOMATON_VAR:
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9') ||
	c == '_') {
      /* Continue. */
      scan_string_append (c);
      break;
    }

    {
      /* Terminate with null. */
      scan_string_append (0);
      char* string;
      size_t size;
      scan_string_steal (&string, &size);
      push_token (AUTOMATON_VAR, string, size);
    }
    
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
      scan_state = SCAN_ERROR;
      break;
    }
    break;

  case SCAN_BINDING_VAR:
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9') ||
	c == '_') {
      /* Continue. */
      scan_string_append (c);
      break;
    }

    {
      /* Terminate with null. */
      scan_string_append (0);
      char* string;
      size_t size;
      scan_string_steal (&string, &size);
      push_token (BINDING_VAR, string, size);
    }
    
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
      scan_state = SCAN_ERROR;
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

  case SCAN_ERROR:
    switch (c) {
    case ';':
    case '\n':
      {
	char* string;
	size_t size;
	scan_string_steal (&string, &size);
	syslog ("syntax error near: ");
	syslogn (string, size);
	free (string);
	scan_state = SCAN_START;
      }
      break;
    default:
      scan_string_append (c);
      break;
    }
    break;
  }
}

/*
  End Scanner Section
  ===================
*/

/*
  Begin Automaton Section
  =======================
*/

#define DESTROY_BUFFERS_NO 1
#define VFS_RESPONSE_NO 2
#define VFS_REQUEST_NO 3
#define LOAD_TEXT_NO 4
#define PROCESS_TEXT_NO 5
#define STDIN_NO 6
#define STDOUT_NO 7
#define START_NO 8
#define STDIN_COL_NO 9
#define INIT_NO 10

#define VFS_NAME "vfs"

/* Initialization flag. */
static bool initialized = false;

/* The aid of the vfs. */
static aid_t vfs_aid = -1;

/* Buffers to interpret. */
static buffer_queue_t interpret_queue;

/* Text to interpret. */
static bd_t interpret_bd = -1;
static const char* interpret_string;
static size_t interpret_string_size;
static size_t interpret_string_idx;

/* Text to output. */
static bd_t stdout_bd;
static buffer_file_t stdout_bf;

static void
readscript_callback (void* data,
		     bd_t bda,
		     bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) != 0) {
    syslog ("jsh: error: vfs provide bad readfile response");
    exit ();
  }

  if (error != VFS_SUCCESS) {
    syslog ("jsh: error: readfile failed");
    exit ();
  }

  /* Put the file on the interpret queue. */
  buffer_queue_push (&interpret_queue, 0, -1, 0, buffer_copy (bdb), size);
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    /* Create an automaton to represent the shell. */
    create_automaton (getaid (), "@this", 6);

    vfs_request_bda = buffer_create (0);
    if (vfs_request_bda == -1) {
      syslog ("jsh: error: Could not create vfs output buffer");
      exit ();
    }
    vfs_request_bdb = buffer_create (0);
    if (vfs_request_bdb == -1) {
      syslog ("jsh: error: Could not create vfs output buffer");
      exit ();
    }
    vfs_request_queue_init (&vfs_request_queue);
    callback_queue_init (&vfs_response_queue);
    buffer_queue_init (&interpret_queue);
    scan_string_init ();

    stdout_bd = buffer_create (0);
    if (stdout_bd == -1) {
      syslog ("jsh: error: Could not create stdout buffer");
      exit ();
    }
    if (buffer_file_initw (&stdout_bf, stdout_bd) != 0) {
      syslog ("jsh: error: Could not initialize stdout buffer");
      exit ();
    }

    aid_t aid = getaid ();
    bd_t bda = getinita ();
    bd_t bdb = getinitb ();

    /* Bind to the vfs. */
    vfs_aid = lookup (VFS_NAME, strlen (VFS_NAME) + 1);
    if (vfs_aid == -1) {
      syslog ("jsh: error: no vfs");
      exit ();
    }
    
    description_t vfs_description;
    if (description_init (&vfs_description, vfs_aid) != 0) {
      syslog ("jsh: error: couldn't describe vfs");
      exit ();
    }
    const ano_t vfs_request = description_name_to_number (&vfs_description, VFS_REQUEST_NAME, strlen (VFS_REQUEST_NAME) + 1);
    const ano_t vfs_response = description_name_to_number (&vfs_description, VFS_RESPONSE_NAME, strlen (VFS_RESPONSE_NAME) + 1);
    description_fini (&vfs_description);
    
    if (vfs_request == NO_ACTION ||
	vfs_response == NO_ACTION) {
      syslog ("jsh: error: the vfs does not appear to be a vfs");
      exit ();
    }
    
    /* We bind the response first so they don't get lost. */
    if (bind (vfs_aid, vfs_response, 0, aid, VFS_RESPONSE_NO, 0) == -1 ||
	bind (aid, VFS_REQUEST_NO, 0, vfs_aid, vfs_request, 0) == -1) {
      syslog ("jsh: error: Couldn't bind to vfs");
      exit ();
    }
    
    argv_t argv;
    size_t argc;
    if (argv_initr (&argv, bda, bdb, &argc) != -1) {
      if (argc >= 2) {
	/* Request the script. */
	const char* filename;
	size_t size;
	if (argv_arg (&argv, 1, (const void**)&filename, &size) != 0) {
	  syslog ("jsh: error: Couldn't read filename argument");
	  exit ();
	}
	
	vfs_request_queue_push_readfile (&vfs_request_queue, filename);
	callback_queue_push (&vfs_response_queue, readscript_callback, 0);
      }
    }
    
    if (bda != -1) {
      buffer_destroy (bda);
    }
    if (bdb != -1) {
      buffer_destroy (bdb);
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  end_internal_action ();
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

BEGIN_OUTPUT (NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (VFS_REQUEST_NO, param);

  if (vfs_request_precondition ()) {
    if (vfs_request_queue_pop_to_buffer (&vfs_request_queue, vfs_request_bda, vfs_request_bdb) != 0) {
      syslog ("jsh: error: Could not write to output buffer");
      exit ();
    }
    end_output_action (true, vfs_request_bda, vfs_request_bdb);
  }
  else {
    end_output_action (false, -1, -1);
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
    syslog ("jsh: error: vfs produced spurious response");
    exit ();
  }

  const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
  callback_t callback = callback_queue_item_callback (item);
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&vfs_response_queue);

  callback (data, bda, bdb);

  end_input_action (bda, bdb);
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

BEGIN_INTERNAL (NO_PARAMETER, LOAD_TEXT_NO, "", "", load_text, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (LOAD_TEXT_NO, param);

  if (load_text_precondition ()) {
    const buffer_queue_item_t* item = buffer_queue_front (&interpret_queue);
    bd_t bda = buffer_queue_item_bda (item);
    if (bda != -1) {
      buffer_destroy (bda);
    }
    bd_t bdb = buffer_queue_item_bdb (item);
    size_t sizeb = buffer_queue_item_sizeb (item);
    buffer_queue_pop (&interpret_queue);

    if (bdb == -1) {
      end_internal_action ();
    }

    size_t bdb_bd_size = buffer_size (bdb);
    if (bdb_bd_size == -1) {
      end_internal_action ();
    }

    if (sizeb > bdb_bd_size * pagesize ()) {
      if (bdb != -1) {
	buffer_destroy (bdb);
      }
      end_internal_action ();
    }

    const char* s = buffer_map (bdb);
    if (s == 0) {
      if (bdb != -1) {
	buffer_destroy (bdb);
      }
      end_internal_action ();
    }

    interpret_bd = bdb;
    interpret_string = s;
    interpret_string_size = sizeb;
    interpret_string_idx = 0;
  }

  end_internal_action ();
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

BEGIN_INTERNAL (NO_PARAMETER, PROCESS_TEXT_NO, "", "", process_text, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (PROCESS_TEXT_NO, param);

  if (process_text_precondition ()) {
    while (!evaluating && interpret_string_idx != interpret_string_size) {
      put (interpret_string[interpret_string_idx++]);
    }

    if (interpret_string_idx == interpret_string_size) {
      /* Processed the complete buffer. */
      buffer_destroy (interpret_bd);
      interpret_bd = -1;
    }
  }

  end_internal_action ();
}

/* stdin
   -----
   Incoming text (to interpret).

   Post: ???
 */
BEGIN_INPUT (NO_PARAMETER, STDIN_NO, "stdin", "buffer_file_t", stdin, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  /* TODO */

  end_input_action (bda, bdb);
}

/* stdout
   ------
   Outgoing text.
   
   Pre:  the output buffer is not empty
   Post: the output buffer appears empty
 */
static bool
stdout_precondition (void)
{
  return buffer_file_size (&stdout_bf) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, STDOUT_NO, "stdout", "buffer_file_t", stdout, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (STDOUT_NO, param);

  if (stdout_precondition ()) {
    buffer_file_truncate (&stdout_bf);
    end_output_action (true, stdout_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

/* start
   -----
   Action for starting automata.
   
   Pre:  ???
   Post: ???
 */
BEGIN_OUTPUT (AUTO_PARAMETER, START_NO, "start", "", start, ano_t ano, aid_t aid)
{
  initialize ();
  scheduler_remove (START_NO, aid);

  start_queue_item_t** item = start_queue_find (aid);
  if (*item != 0) {
    start_queue_erase (item);
    end_output_action (true, -1, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

/* stdout_col
   ----------
   Standard output collector.

   Post: ???
 */
BEGIN_INPUT (AUTO_PARAMETER, STDIN_COL_NO, "stdin_col", "buffer_file_t", stdin_col, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t bf;
  if (buffer_file_initr (&bf, bda) != 0) {
    end_input_action (bda, bdb);
  }

  size_t size = buffer_file_size (&bf);
  const char* data = buffer_file_readp (&bf, size);
  if (data == 0) {
    end_input_action (bda, bdb);
  }

  buffer_file_write (&stdout_bf, data, size);

  end_input_action (bda, bdb);
}

void
schedule (void)
{
  if (vfs_request_precondition ()) {
    scheduler_add (VFS_REQUEST_NO, 0);
  }
  if (load_text_precondition ()) {
    scheduler_add (LOAD_TEXT_NO, 0);
  }
  if (process_text_precondition ()) {
    scheduler_add (PROCESS_TEXT_NO, 0);
  }
  if (stdout_precondition ()) {
    scheduler_add (STDOUT_NO, 0);
  }
  if (!start_queue_empty ()) {
    scheduler_add (START_NO, start_queue_front ());
  }
}

/*
  End Automaton Section
  =======================
*/
