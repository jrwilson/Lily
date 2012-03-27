#include <automaton.h>
#include <string.h>
#include <buffer_queue.h>
#include <callback_queue.h>
#include <description.h>
#include <dymem.h>
#include "vfs_msg.h"
#include "argv.h"
#include "syslog.h"

/* TODO:  This is wrong on multiple levels. */
static int
atoi (const char* s)
{
  int retval = 0;

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

  return retval;
}

/* Initialization flag. */
static bool initialized = false;

typedef enum {
  RUN,
  STOP,
} state_t;
static state_t state = RUN;

/* Syslog buffer. */
static bd_t syslog_bd = -1;
static buffer_file_t syslog_buffer;

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

/* Messages to the vfs. */
static bd_t vfs_request_bda = -1;
static bd_t vfs_request_bdb = -1;
static vfs_request_queue_t vfs_request_queue;

/* Callbacks to execute from the vfs. */
static callback_queue_t vfs_response_queue;

#define INFO "jsh: info: "
#define WARNING "jsh: warning: "
#define ERROR "jsh: error: "

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
    bfprintf (&syslog_buffer, ERROR "could create argv\n");
    state = STOP;
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
    bfprintf (&syslog_buffer, ERROR "vfs provide bad readfile response\n");
    state = STOP;
    return;
  }

  if (error == VFS_SUCCESS) {
    aid_t aid = create (bdb, size, cc->bda, cc->bdb, cc->registry_name, cc->registry_name_size, cc->retain_privilege);
    if (aid != -1) {
      /*      bfprintf (&syslog_buffer, INFO "TODO:  Subscribe to created automaton\n"); */
      /* Assign the result to a variable. */
      create_automaton (aid, cc->name, cc->name_size);
    }
    else {
      bfprintf (&syslog_buffer, INFO "TODO:  create create failed\n");
    }
  }
  else {
    bfprintf (&syslog_buffer, INFO "TODO:  create couldn't read file\n");
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
  LPAREN,
  RPAREN,
  PIPE,
  TAG,
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

  /* Correct the type. */
  if (type == STRING) {
    if (strncmp ("tag", string, size) == 0) {
      item->type = TAG;
    }
  }
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

static bool
match (token_type_t type)
{
  if (current_token != 0 && current_token->type == type) {
    current_token = current_token->next;
    return true;
  }
  else {
    return false;
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
    bfprintf (&syslog_buffer, INFO "TODO: automaton variable already assigned\n");
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
	bfprintf (&syslog_buffer, INFO "TODO: expected a name\n");
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
    bfprintf (&syslog_buffer, INFO "TODO: expected a filename or argument\n");
    return;
  }

  token_list_item_t* string;
  for (string = current_token; string != 0; string = string->next) {
    if (string->type != STRING) {
      bfprintf (&syslog_buffer, INFO "TODO: expected a string\n");
      return;
    }
  }

  /* Create context for the create callback. */
  create_context_t* cc = create_create_context (var->string, var->size, retain_privilege, registry_name, registry_name_size);

  /* Append the filename. */
  argv_append (&cc->argv, filename->string, filename->size);

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
    bfprintf (&syslog_buffer, INFO "TODO: binding variable already assigned\n");
    return;
  }

  int output_parameter = 0;
  int input_parameter = 0;

  /* Get the parameter tokens. */
  while (current_token != 0 &&
	 current_token->type == STRING) {
    if (strcmp (current_token->string, "-p") == 0) {
      accept (STRING);
      token_list_item_t* parameter_token;
      if ((parameter_token = accept (STRING)) == 0) {
	bfprintf (&syslog_buffer, INFO "TODO: expected a parameter\n");
	return;
      }
      output_parameter = atoi (parameter_token->string);
    }
    else if (strcmp (current_token->string, "-q") == 0) {
      accept (STRING);
      token_list_item_t* parameter_token;
      if ((parameter_token = accept (STRING)) == 0) {
	bfprintf (&syslog_buffer, INFO "TODO: expected a parameter\n");
	return;
      }
      input_parameter = atoi (parameter_token->string);
    }
    else {
      break;
    }
  }

  /* Get the argument tokens. */
  token_list_item_t* output_automaton_token;
  token_list_item_t* output_action_token;
  token_list_item_t* input_automaton_token;
  token_list_item_t* input_action_token;
  if ((output_automaton_token = accept (AUTOMATON_VAR)) == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected an automaton variable\n");
    return;
  }

  if ((output_action_token = accept (STRING)) == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected a string\n");
    return;
  }

  if ((input_automaton_token = accept (AUTOMATON_VAR)) == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected an automaton variable\n");
    return;
  }

  if ((input_action_token = accept (STRING)) == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected a string\n");
    return;
  }

  if (current_token != 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected no more tokens\n");
    return;
  }

  /* Look up the variables for the output and input automaton. */
  automaton_t* output_automaton = find_automaton (output_automaton_token->string, output_automaton_token->size);
  if (output_automaton == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Output automaton is not defined\n");
    return;
  }
  automaton_t* input_automaton = find_automaton (input_automaton_token->string, input_automaton_token->size);
  if (input_automaton == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Input automaton is not defined\n");
    return;
  }

  /* Describe the output and input automaton. */
  description_t output_description;
  description_t input_description;
  if (description_init (&output_description, output_automaton->aid) != 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Could not describe output\n");
    return;
  }
  if (description_init (&input_description, input_automaton->aid) != 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Could not describe input\n");
    return;
  }

  /* Look up the actions. */
  const ano_t output_action = description_name_to_number (&output_description, output_action_token->string, output_action_token->size);
  const ano_t input_action = description_name_to_number (&input_description, input_action_token->string, input_action_token->size);

  description_fini (&output_description);
  description_fini (&input_description);

  if (output_action == -1) {
    bfprintf (&syslog_buffer, INFO "TODO:  Output action does not exist\n");
    return;
  }

  if (input_action == -1) {
    bfprintf (&syslog_buffer, INFO "TODO:  Input action does not exist\n");
    return;
  }

  bid_t bid = bind (output_automaton->aid, output_action, output_parameter, input_automaton->aid, input_action, input_parameter);
  if (bid == -1) {
    bfprintf (&syslog_buffer, INFO "TODO:  Bind failed\n");
    return;
  }

  if (var == 0) {
    /*    bfprintf (&syslog_buffer, INFO "TODO:  Create binding\n"); */
    /* char buffer[sizeof (bid_t) * 8]; */
    /* size_t sz = snprintf (buffer, sizeof (bid_t) * 8, "&b%d", bid); */
    /* create_binding (bid, buffer, sz + 1); */
  }
  else {
    create_binding (bid, var->string, var->size);
  }
  
  /*  bfprintf (&syslog_buffer, INFO "TODO:  Subscribe to binding??\n"); */
}

static void
unbind_ ()
{
  /* Get the argument tokens. */
  token_list_item_t* binding_token;
  if ((binding_token = accept (BINDING_VAR)) == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected a binding variable\n");
    return;
  }

  if (current_token != 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected no more tokens\n");
    return;
  }

  /* Look up the binding variable. */
  binding_t* binding = find_binding (binding_token->string, binding_token->size);
  if (binding == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Binding is not defined\n");
    return;
  }

  /*  bfprintf (&syslog_buffer, INFO "TODO:  Unsubscribe from binding??\n"); */

  if (unbind (binding->bid) != 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  unbind failed\n");
  }

  destroy_binding (binding);
}

static void
destroy_ ()
{
  /* Get the argument tokens. */
  token_list_item_t* automaton_token;
  if ((automaton_token = accept (AUTOMATON_VAR)) == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected an automaton variable\n");
    return;
  }

  if (current_token != 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected no more tokens\n");
    return;
  }

  /* Look up the automaton variable. */
  automaton_t* automaton = find_automaton (automaton_token->string, automaton_token->size);
  if (automaton == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Automaton is not defined\n");
    return;
  }

  /*  bfprintf (&syslog_buffer, INFO "TODO:  Unsubscribe from automaton??\n"); */

  if (destroy (automaton->aid) != 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  destroy failed\n");
  }

  destroy_automaton (automaton);
}

static void
start_ (void)
{
  /* Get the argument token. */
  token_list_item_t* automaton_token;
  if ((automaton_token = accept (AUTOMATON_VAR)) == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected an automaton variable\n");
    return;
  }

  if (current_token != 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected no more tokens\n");
    return;
  }

  /* Look up the variables for the automaton. */
  automaton_t* automaton = find_automaton (automaton_token->string, automaton_token->size);
  if (automaton == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Automaton is not defined\n");
    return;
  }

  /* TODO:  If we are not bound to it, then we should probably warn them. */

  start_queue_push (automaton->aid);
}

static void
lookup_ (token_list_item_t* var)
{
  /* Get the name. */
  token_list_item_t* name_token;
  if ((name_token = accept (STRING)) == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected a name\n");
  }

  if (current_token != 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected no more tokens\n");
    return;
  }

  aid_t aid = lookup (name_token->string, name_token->size);
  if (aid != -1) {
    /* Assign the result to a variable. */
    create_automaton (aid, var->string, var->size);
  }
  else {
    // TODO:  Automaton does not exist.
  }
}

static void
automaton_assignment (token_list_item_t* var)
{
  if (accept (ASSIGN) == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected =\n");
    return;
  }
  
  token_list_item_t* t;
  if ((t = accept (STRING)) != 0) {
    if (strncmp ("create", t->string, t->size) == 0) {
      create_ (var);
    }
    else if (strncmp ("lookup", t->string, t->size) == 0) {
      lookup_ (var);
    }
    else {
      bfprintf (&syslog_buffer, INFO "TODO:  syntax error\n");
    }
  }
  else {
    bfprintf (&syslog_buffer, INFO "TODO:  syntax error\n");
  }
}

static void
binding_assignment (token_list_item_t* var)
{
  if (accept (ASSIGN) == 0) {
    bfprintf (&syslog_buffer, INFO "TODO:  Expected =\n");
    return;
  }
  
  token_list_item_t* t;
  if ((t = accept (STRING)) != 0 && strncmp ("bind", t->string, t->size) == 0) {
    bind_ (var);
  }
  else {
    bfprintf (&syslog_buffer, INFO "TODO:  syntax error\n");
  }
}

/* static void */
/* tag (void) */
/* { */
/*   match (TAG); */
/*   expression (); */
  
/* } */

static bool
expression0 (void)
{
  /* if (current_token->type == TAG) { */
  /*   tag (); */
  /*   return; */
  /* } */

  token_list_item_t* t;

  if ((t = accept (AUTOMATON_VAR)) != 0) {
    automaton_assignment (t);
    return true;
  }
  else if ((t = accept (BINDING_VAR)) != 0) {
    binding_assignment (t);
    return true;
  }
  else if ((t = accept (STRING)) != 0) {
    if (strncmp ("bind", t->string, t->size) == 0) {
      bind_ (0);
      return true;
    }
    else if (strncmp ("unbind", t->string, t->size) == 0) {
      unbind_ ();
      return true;
    }
    else if (strncmp ("destroy", t->string, t->size) == 0) {
      destroy_ ();
      return true;
    }
    else if (strncmp ("start", t->string, t->size) == 0) {
      start_ ();
      return true;
    }
  }

  bfprintf (&syslog_buffer, INFO "TODO:  syntax error\n");
  return false;
}

static void
expression1_alpha (void)
{
  expression0 ();
}

static bool
expression1_beta (void)
{
  if (current_token == 0) {
    return true;
  }

  if (!match (PIPE)) {
    bfprintf (&syslog_buffer, INFO "TODO:  expected |\n");
    return false;
  }
  if (!expression0 ()) {
    return false;
  }
  if (!expression1_beta ()) {
    return false;
  }

  return true;
}

static void
expression1 (void)
{
  expression1_alpha ();
  expression1_beta ();
}

static void
evaluate (void)
{
  if (token_list_head != 0) {
    /* Try to evaluate a statement. */
    current_token = token_list_head;
    expression1 ();
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

   string - [a-zA-Z0-9_/-.]+

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
	c == '-' ||
	c == '.') {
      scan_string_append (c);
      scan_state = SCAN_STRING;
      break;
    }

    switch (c) {
    case '(':
      push_token (LPAREN, 0, 0);
      break;
    case ')':
      push_token (RPAREN, 0, 0);
      break;
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
	c == '-' ||
	c == '.') {
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
    case '(':
      push_token (LPAREN, 0, 0);
      scan_state = SCAN_START;
      break;
    case ')':
      push_token (RPAREN, 0, 0);
      scan_state = SCAN_START;
      break;
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
    case '(':
      push_token (LPAREN, 0, 0);
      scan_state = SCAN_START;
      break;
    case ')':
      push_token (RPAREN, 0, 0);
      scan_state = SCAN_START;
      break;
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
    case '(':
      push_token (LPAREN, 0, 0);
      scan_state = SCAN_START;
      break;
    case ')':
      push_token (RPAREN, 0, 0);
      scan_state = SCAN_START;
      break;
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
	bfprintf (&syslog_buffer, INFO "syntax error near: ");
	buffer_file_write (&syslog_buffer, string, size);
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

#define INIT_NO 1
#define STOP_NO 2
#define SYSLOG_NO 3
#define DESTROY_BUFFERS_NO 4
#define VFS_RESPONSE_NO 5
#define VFS_REQUEST_NO 6
#define LOAD_TEXT_NO 7
#define PROCESS_TEXT_NO 8
#define STDIN_NO 9
#define STDOUT_NO 10
#define START_NO 11
#define STDIN_COL_NO 12

#define VFS_NAME "vfs"

static void
readscript_callback (void* data,
		     bd_t bda,
		     bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) != 0) {
    bfprintf (&syslog_buffer, ERROR "vfs provide bad readfile response\n");
    state = STOP;
    return;
  }

  if (error != VFS_SUCCESS) {
    bfprintf (&syslog_buffer, ERROR "readfile failed\n");
    state = STOP;
    return;
  }

  /* Put the file on the interpret queue. */
  buffer_queue_push (&interpret_queue, 0, -1, 0, buffer_copy (bdb), size);
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    syslog_bd = buffer_create (0);
    if (syslog_bd == -1) {
      /* Nothing we can do. */
      exit ();
    }
    if (buffer_file_initw (&syslog_buffer, syslog_bd) != 0) {
      /* Nothing we can do. */
      exit ();
    }

    aid_t syslog_aid = lookup (SYSLOG_NAME, strlen (SYSLOG_NAME) + 1);
    if (syslog_aid != -1) {
      /* Bind to the syslog. */

      description_t syslog_description;
      if (description_init (&syslog_description, syslog_aid) != 0) {
	exit ();
      }
      
      const ano_t syslog_stdin = description_name_to_number (&syslog_description, SYSLOG_STDIN, strlen (SYSLOG_STDIN) + 1);
      
      description_fini (&syslog_description);
      
      /* We bind the response first so they don't get lost. */
      if (bind (getaid (), SYSLOG_NO, 0, syslog_aid, syslog_stdin, 0) == -1) {
	exit ();
      }
    }

    /* Create an automaton to represent the shell. */
    create_automaton (getaid (), "@this", 6);

    vfs_request_bda = buffer_create (0);
    vfs_request_bdb = buffer_create (0);
    if (vfs_request_bda == -1 ||
	vfs_request_bdb == -1) {
      bfprintf (&syslog_buffer, ERROR "could not create vfs output buffer\n");
      state = STOP;
      return;
    }
    vfs_request_queue_init (&vfs_request_queue);
    callback_queue_init (&vfs_response_queue);
    buffer_queue_init (&interpret_queue);
    scan_string_init ();

    stdout_bd = buffer_create (0);
    if (stdout_bd == -1) {
      bfprintf (&syslog_buffer, ERROR "could not create stdout buffer\n");
      state = STOP;
      return;
    }
    if (buffer_file_initw (&stdout_bf, stdout_bd) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not initialize stdout buffer\n");
      state = STOP;
      return;
    }

    aid_t aid = getaid ();
    bd_t bda = getinita ();
    bd_t bdb = getinitb ();

    /* Bind to the vfs. */
    vfs_aid = lookup (VFS_NAME, strlen (VFS_NAME) + 1);
    if (vfs_aid == -1) {
      bfprintf (&syslog_buffer, ERROR "no vfs\n");
      state = STOP;
      return;
    }
    
    description_t vfs_description;
    if (description_init (&vfs_description, vfs_aid) != 0) {
      bfprintf (&syslog_buffer, ERROR "couldn't describe vfs\n");
      state = STOP;
      return;
    }
    const ano_t vfs_request = description_name_to_number (&vfs_description, VFS_REQUEST_NAME, strlen (VFS_REQUEST_NAME) + 1);
    const ano_t vfs_response = description_name_to_number (&vfs_description, VFS_RESPONSE_NAME, strlen (VFS_RESPONSE_NAME) + 1);
    description_fini (&vfs_description);
    
    if (vfs_request == -1 ||
	vfs_response == -1) {
      bfprintf (&syslog_buffer, ERROR "vfs does not appear to be a vfs\n");
      state = STOP;
      return;
    }
    
    /* We bind the response first so they don't get lost. */
    if (bind (vfs_aid, vfs_response, 0, aid, VFS_RESPONSE_NO, 0) == -1 ||
	bind (aid, VFS_REQUEST_NO, 0, vfs_aid, vfs_request, 0) == -1) {
      bfprintf (&syslog_buffer, ERROR "couldn't bind to vfs\n");
      state = STOP;
      return;
    }
    
    argv_t argv;
    size_t argc;
    if (argv_initr (&argv, bda, bdb, &argc) != -1) {
      if (argc >= 2) {
	/* Request the script. */
	const char* filename;
	size_t size;
	if (argv_arg (&argv, 1, (const void**)&filename, &size) != 0) {
	  bfprintf (&syslog_buffer, ERROR "couldn't read filename argument\n");
	  state = STOP;
	  return;
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

  if (vfs_request_precondition ()) {
    if (vfs_request_queue_pop_to_buffer (&vfs_request_queue, vfs_request_bda, vfs_request_bdb) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not write to output buffer\n");
      state = STOP;
      finish_output (false, -1, -1);
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
    bfprintf (&syslog_buffer, WARNING "vfs produced spurious response\n");
    finish_input (bda, bdb);
  }

  const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
  callback_t callback = callback_queue_item_callback (item);
  if (!(callback == readscript_callback || callback == create_callback)) {
    bfprintf (&syslog_buffer, ERROR "not an appropriate callback\n");
    state = STOP;
    finish_input (bda, bdb);
  }
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&vfs_response_queue);

  callback (data, bda, bdb);

  finish_input (bda, bdb);
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
      finish_internal ();
    }

    size_t bdb_bd_size = buffer_size (bdb);
    if (bdb_bd_size == -1) {
      finish_internal ();
    }

    if (sizeb > bdb_bd_size * pagesize ()) {
      if (bdb != -1) {
	buffer_destroy (bdb);
      }
      finish_internal ();
    }

    const char* s = buffer_map (bdb);
    if (s == 0) {
      if (bdb != -1) {
	buffer_destroy (bdb);
      }
      finish_internal ();
    }

    interpret_bd = bdb;
    interpret_string = s;
    interpret_string_size = sizeb;
    interpret_string_idx = 0;
  }

  finish_internal ();
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

  finish_internal ();
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

  finish_input (bda, bdb);
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

  if (stdout_precondition ()) {
    buffer_file_truncate (&stdout_bf);
    finish_output (true, stdout_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
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

  start_queue_item_t** item = start_queue_find (aid);
  if (*item != 0) {
    start_queue_erase (item);
    finish_output (true, -1, -1);
  }
  else {
    finish_output (false, -1, -1);
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
    finish_input (bda, bdb);
  }

  size_t size = buffer_file_size (&bf);
  const char* data = buffer_file_readp (&bf, size);
  if (data == 0) {
    finish_input (bda, bdb);
  }

  buffer_file_write (&stdout_bf, data, size);

  finish_input (bda, bdb);
}

void
do_schedule (void)
{
  if (stop_precondition ()) {
    schedule (STOP_NO, 0);
  }
  if (syslog_precondition ()) {
    schedule (SYSLOG_NO, 0);
  }
  if (vfs_request_precondition ()) {
    schedule (VFS_REQUEST_NO, 0);
  }
  if (load_text_precondition ()) {
    schedule (LOAD_TEXT_NO, 0);
  }
  if (process_text_precondition ()) {
    schedule (PROCESS_TEXT_NO, 0);
  }
  if (stdout_precondition ()) {
    schedule (STDOUT_NO, 0);
  }
  if (!start_queue_empty ()) {
    schedule (START_NO, start_queue_front ());
  }
}

/*
  End Automaton Section
  =======================
*/
