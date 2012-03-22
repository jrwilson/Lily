#include <automaton.h>
#include <buffer_file.h>
#include <dymem.h>
#include <string.h>

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

#define INIT_NO 1
#define START_NO 2
#define STDIN_NO 3
#define STDOUT_NO 4

/* Initialization flag. */
static bool initialized = false;

/* Started flag. */
static bool started = false;

static bd_t stdout_bd = 1;
static buffer_file_t stdout_buffer;

#define PROMPT "$ "

/*
  Begin Parser Section
  ==============================
*/

typedef enum {
  END,

  LPAREN,
  RPAREN,
  COMMA,
 SEMICOLON,
  PIPE,

  CREATE,
  LOOKUP,
  BIND,
  TAG,
  UNTAG,
  DESTROY,
  UNBIND,
  START,

  AID,
  BID,
  STRING,
} token_type_t;

typedef struct token_list_item token_list_item_t;
struct token_list_item {
  token_type_t type;
  char* string;
  size_t size;
  int val;
  token_list_item_t* next;
};

static token_list_item_t* token_list_head = 0;
static token_list_item_t** token_list_tail = &token_list_head;
static token_list_item_t* current_token;

static void
push_token (token_type_t type,
	    char* string,
	    size_t size,
	    int val)
{
  token_list_item_t* item = malloc (sizeof (token_list_item_t));
  memset (item, 0, sizeof (token_list_item_t));
  item->type = type;
  item->string = string;
  item->size = size;
  item->val = val;
  *token_list_tail = item;
  token_list_tail = &item->next;

  /* Find keywords. */
  if (type == STRING) {
    if (strncmp ("create", string, size) == 0) {
      item->type = CREATE;
    }
    else if (strncmp ("lookup", string, size) == 0) {
      item->type = LOOKUP;
    }
    else if (strncmp ("bind", string, size) == 0) {
      item->type = BIND;
    }
    else if (strncmp ("tag", string, size) == 0) {
      item->type = TAG;
    }
    else if (strncmp ("untag", string, size) == 0) {
      item->type = UNTAG;
    }
    else if (strncmp ("destroy", string, size) == 0) {
      item->type = DESTROY;
    }
    else if (strncmp ("unbind", string, size) == 0) {
      item->type = UNBIND;
    }
    else if (strncmp ("start", string, size) == 0) {
      item->type = START;
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

/* static void */
/* create_ (token_list_item_t* var) */
/* { */
/*   bool retain_privilege = false; */
/*   const char* registry_name = 0; */
/*   size_t registry_name_size = 0; */
/*   token_list_item_t* filename; */
  
/*   if (find_automaton (var->string, var->size) != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO: automaton variable already assigned\n"); */
/*     return; */
/*   } */

/*   while (current_token != 0 && */
/* 	 current_token->type == STRING) { */
/*     if (strcmp (current_token->string, "-p") == 0) { */
/*       retain_privilege = true; */
/*       accept (STRING); */
/*     } */
/*     else if (strcmp (current_token->string, "-n") == 0) { */
/*       accept (STRING); */
/*       token_list_item_t* reg_name; */
/*       if ((reg_name = accept (STRING)) == 0) { */
/* 	bfprintf (&syslog_buffer, INFO "TODO: expected a name\n"); */
/* 	return; */
/*       } */
/*       registry_name = reg_name->string; */
/*       registry_name_size = reg_name->size; */
/*     } */
/*     else { */
/*       break; */
/*     } */
/*   } */

/*   if ((filename = accept (STRING)) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO: expected a filename or argument\n"); */
/*     return; */
/*   } */

/*   token_list_item_t* string; */
/*   for (string = current_token; string != 0; string = string->next) { */
/*     if (string->type != STRING) { */
/*       bfprintf (&syslog_buffer, INFO "TODO: expected a string\n"); */
/*       return; */
/*     } */
/*   } */

/*   /\* Create context for the create callback. *\/ */
/*   create_context_t* cc = create_create_context (var->string, var->size, retain_privilege, registry_name, registry_name_size); */
  
/*   while ((string = accept (STRING)) != 0) { */
/*     /\* Add the string to argv. *\/ */
/*     argv_append (&cc->argv, string->string, string->size); */
/*   } */
  
/*   /\* Request the text of the automaton. *\/ */
/*   vfs_request_queue_push_readfile (&vfs_request_queue, filename->string); */
/*   callback_queue_push (&vfs_response_queue, create_callback, cc); */
/*   /\* Set the flag so we stop trying to evaluate. *\/ */
/*   evaluating = true; */
/* } */

/* static void */
/* bind_ (token_list_item_t* var) */
/* { */
/*   if (var != 0 && find_binding (var->string, var->size) != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO: binding variable already assigned\n"); */
/*     return; */
/*   } */

/*   int output_parameter = 0; */
/*   int input_parameter = 0; */

/*   /\* Get the parameter tokens. *\/ */
/*   while (current_token != 0 && */
/* 	 current_token->type == STRING) { */
/*     if (strcmp (current_token->string, "-p") == 0) { */
/*       accept (STRING); */
/*       token_list_item_t* parameter_token; */
/*       if ((parameter_token = accept (STRING)) == 0) { */
/* 	bfprintf (&syslog_buffer, INFO "TODO: expected a parameter\n"); */
/* 	return; */
/*       } */
/*       output_parameter = atoi (parameter_token->string); */
/*     } */
/*     else if (strcmp (current_token->string, "-q") == 0) { */
/*       accept (STRING); */
/*       token_list_item_t* parameter_token; */
/*       if ((parameter_token = accept (STRING)) == 0) { */
/* 	bfprintf (&syslog_buffer, INFO "TODO: expected a parameter\n"); */
/* 	return; */
/*       } */
/*       input_parameter = atoi (parameter_token->string); */
/*     } */
/*     else { */
/*       break; */
/*     } */
/*   } */

/*   /\* Get the argument tokens. *\/ */
/*   token_list_item_t* output_automaton_token; */
/*   token_list_item_t* output_action_token; */
/*   token_list_item_t* input_automaton_token; */
/*   token_list_item_t* input_action_token; */
/*   if ((output_automaton_token = accept (AUTOMATON_VAR)) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected an automaton variable\n"); */
/*     return; */
/*   } */

/*   if ((output_action_token = accept (STRING)) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected a string\n"); */
/*     return; */
/*   } */

/*   if ((input_automaton_token = accept (AUTOMATON_VAR)) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected an automaton variable\n"); */
/*     return; */
/*   } */

/*   if ((input_action_token = accept (STRING)) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected a string\n"); */
/*     return; */
/*   } */

/*   if (current_token != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected no more tokens\n"); */
/*     return; */
/*   } */

/*   /\* Look up the variables for the output and input automaton. *\/ */
/*   automaton_t* output_automaton = find_automaton (output_automaton_token->string, output_automaton_token->size); */
/*   if (output_automaton == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Output automaton is not defined\n"); */
/*     return; */
/*   } */
/*   automaton_t* input_automaton = find_automaton (input_automaton_token->string, input_automaton_token->size); */
/*   if (input_automaton == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Input automaton is not defined\n"); */
/*     return; */
/*   } */

/*   /\* Describe the output and input automaton. *\/ */
/*   description_t output_description; */
/*   description_t input_description; */
/*   if (description_init (&output_description, output_automaton->aid) != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Could not describe output\n"); */
/*     return; */
/*   } */
/*   if (description_init (&input_description, input_automaton->aid) != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Could not describe input\n"); */
/*     return; */
/*   } */

/*   /\* Look up the actions. *\/ */
/*   const ano_t output_action = description_name_to_number (&output_description, output_action_token->string, output_action_token->size); */
/*   const ano_t input_action = description_name_to_number (&input_description, input_action_token->string, input_action_token->size); */

/*   description_fini (&output_description); */
/*   description_fini (&input_description); */

/*   if (output_action == -1) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Output action does not exist\n"); */
/*     return; */
/*   } */

/*   if (input_action == -1) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Input action does not exist\n"); */
/*     return; */
/*   } */

/*   bid_t bid = bind (output_automaton->aid, output_action, output_parameter, input_automaton->aid, input_action, input_parameter); */
/*   if (bid == -1) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Bind failed\n"); */
/*     return; */
/*   } */

/*   if (var == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Create binding\n"); */
/*     /\* char buffer[sizeof (bid_t) * 8]; *\/ */
/*     /\* size_t sz = snprintf (buffer, sizeof (bid_t) * 8, "&b%d", bid); *\/ */
/*     /\* create_binding (bid, buffer, sz + 1); *\/ */
/*   } */
/*   else { */
/*     create_binding (bid, var->string, var->size); */
/*   } */
  
/*   bfprintf (&syslog_buffer, INFO "TODO:  Subscribe to binding??\n"); */
/* } */

/* static void */
/* unbind_ () */
/* { */
/*   /\* Get the argument tokens. *\/ */
/*   token_list_item_t* binding_token; */
/*   if ((binding_token = accept (BINDING_VAR)) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected a binding variable\n"); */
/*     return; */
/*   } */

/*   if (current_token != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected no more tokens\n"); */
/*     return; */
/*   } */

/*   /\* Look up the binding variable. *\/ */
/*   binding_t* binding = find_binding (binding_token->string, binding_token->size); */
/*   if (binding == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Binding is not defined\n"); */
/*     return; */
/*   } */

/*   bfprintf (&syslog_buffer, INFO "TODO:  Unsubscribe from binding??\n"); */

/*   if (unbind (binding->bid) != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  unbind failed\n"); */
/*   } */

/*   destroy_binding (binding); */
/* } */

/* static void */
/* destroy_ () */
/* { */
/*   /\* Get the argument tokens. *\/ */
/*   token_list_item_t* automaton_token; */
/*   if ((automaton_token = accept (AUTOMATON_VAR)) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected an automaton variable\n"); */
/*     return; */
/*   } */

/*   if (current_token != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected no more tokens\n"); */
/*     return; */
/*   } */

/*   /\* Look up the automaton variable. *\/ */
/*   automaton_t* automaton = find_automaton (automaton_token->string, automaton_token->size); */
/*   if (automaton == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Automaton is not defined\n"); */
/*     return; */
/*   } */

/*   bfprintf (&syslog_buffer, INFO "TODO:  Unsubscribe from automaton??\n"); */

/*   if (destroy (automaton->aid) != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  destroy failed\n"); */
/*   } */

/*   destroy_automaton (automaton); */
/* } */

/* static void */
/* start_ (void) */
/* { */
/*   /\* Get the argument token. *\/ */
/*   token_list_item_t* automaton_token; */
/*   if ((automaton_token = accept (AUTOMATON_VAR)) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected an automaton variable\n"); */
/*     return; */
/*   } */

/*   if (current_token != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected no more tokens\n"); */
/*     return; */
/*   } */

/*   /\* Look up the variables for the automaton. *\/ */
/*   automaton_t* automaton = find_automaton (automaton_token->string, automaton_token->size); */
/*   if (automaton == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Automaton is not defined\n"); */
/*     return; */
/*   } */

/*   /\* TODO:  If we are not bound to it, then we should probably warn them. *\/ */

/*   start_queue_push (automaton->aid); */
/* } */

/* static void */
/* lookup_ (token_list_item_t* var) */
/* { */
/*   /\* Get the name. *\/ */
/*   token_list_item_t* name_token; */
/*   if ((name_token = accept (STRING)) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected a name\n"); */
/*   } */

/*   if (current_token != 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected no more tokens\n"); */
/*     return; */
/*   } */

/*   aid_t aid = lookup (name_token->string, name_token->size); */
/*   if (aid != -1) { */
/*     /\* Assign the result to a variable. *\/ */
/*     create_automaton (aid, var->string, var->size); */
/*   } */
/*   else { */
/*     // TODO:  Automaton does not exist. */
/*   } */
/* } */

/* static void */
/* automaton_assignment (token_list_item_t* var) */
/* { */
/*   if (accept (ASSIGN) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected =\n"); */
/*     return; */
/*   } */
  
/*   token_list_item_t* t; */
/*   if ((t = accept (STRING)) != 0) { */
/*     if (strncmp ("create", t->string, t->size) == 0) { */
/*       create_ (var); */
/*     } */
/*     else if (strncmp ("lookup", t->string, t->size) == 0) { */
/*       lookup_ (var); */
/*     } */
/*     else { */
/*       bfprintf (&syslog_buffer, INFO "TODO:  syntax error\n"); */
/*     } */
/*   } */
/*   else { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  syntax error\n"); */
/*   } */
/* } */

/* static void */
/* binding_assignment (token_list_item_t* var) */
/* { */
/*   if (accept (ASSIGN) == 0) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  Expected =\n"); */
/*     return; */
/*   } */
  
/*   token_list_item_t* t; */
/*   if ((t = accept (STRING)) != 0 && strncmp ("bind", t->string, t->size) == 0) { */
/*     bind_ (var); */
/*   } */
/*   else { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  syntax error\n"); */
/*   } */
/* } */

/* /\* static void *\/ */
/* /\* tag (void) *\/ */
/* /\* { *\/ */
/* /\*   match (TAG); *\/ */
/* /\*   expression (); *\/ */
  
/* /\* } *\/ */

/* static bool */
/* expression0 (void) */
/* { */
/*   /\* if (current_token->type == TAG) { *\/ */
/*   /\*   tag (); *\/ */
/*   /\*   return; *\/ */
/*   /\* } *\/ */

/*   token_list_item_t* t; */

/*   if ((t = accept (AUTOMATON_VAR)) != 0) { */
/*     automaton_assignment (t); */
/*     return true; */
/*   } */
/*   else if ((t = accept (BINDING_VAR)) != 0) { */
/*     binding_assignment (t); */
/*     return true; */
/*   } */
/*   else if ((t = accept (STRING)) != 0) { */
/*     if (strncmp ("bind", t->string, t->size) == 0) { */
/*       bind_ (0); */
/*       return true; */
/*     } */
/*     else if (strncmp ("unbind", t->string, t->size) == 0) { */
/*       unbind_ (); */
/*       return true; */
/*     } */
/*     else if (strncmp ("destroy", t->string, t->size) == 0) { */
/*       destroy_ (); */
/*       return true; */
/*     } */
/*     else if (strncmp ("start", t->string, t->size) == 0) { */
/*       start_ (); */
/*       return true; */
/*     } */
/*   } */

/*   bfprintf (&syslog_buffer, INFO "TODO:  syntax error\n"); */
/*   return false; */
/* } */

/* static void */
/* expression1_alpha (void) */
/* { */
/*   expression0 (); */
/* } */

/* static bool */
/* expression1_beta (void) */
/* { */
/*   if (current_token == 0) { */
/*     return true; */
/*   } */

/*   if (!match (PIPE)) { */
/*     bfprintf (&syslog_buffer, INFO "TODO:  expected |\n"); */
/*     return false; */
/*   } */
/*   if (!expression0 ()) { */
/*     return false; */
/*   } */
/*   if (!expression1_beta ()) { */
/*     return false; */
/*   } */

/*   return true; */
/* } */

/* static void */
/* expression1 (void) */
/* { */
/*   expression1_alpha (); */
/*   expression1_beta (); */
/* } */

/* static void */
/* evaluate (void) */
/* { */
/*   if (token_list_head != 0) { */
/*     /\* Try to evaluate a statement. *\/ */
/*     current_token = token_list_head; */
/*     expression1 (); */
/*     clean_tokens (); */
/*   } */
/* } */

/*
  Grammar

  program      -> trailer1 $
               -> expression1 trailer1 $
  trailer1     -> lambda
               -> ; trailer2
  trailer2     -> expression1 trailer1
               -> trailer1
  expression1  -> expression0 expression1b
  expression1b -> | expression0 expression1b
               -> lambda
 */

static bool
trailer1 (void);

static bool
expression1 (void);

static bool
expression0 (void)
{
  bfprintf (&stdout_buffer, "expression0\n");
  switch (current_token->type) {
  case LPAREN:
    match (LPAREN);
    if (!expression1 ()) {
      return false;
    }
    if (!match (RPAREN)) {
      return false;
    }
    return true;
  case CREATE:
    match (CREATE);
    bfprintf (&stdout_buffer, "CREATE\n");
    return true;
  case LOOKUP:
    match (LOOKUP);
    bfprintf (&stdout_buffer, "LOOKUP\n");
    return true;
  case BIND:
    match (BIND);
    bfprintf (&stdout_buffer, "BIND\n");
    return true;
  case TAG:
    match (TAG);
    bfprintf (&stdout_buffer, "TAG\n");
    if (!expression1 ()) {
      return false;
    }
    return true;
  case UNTAG:
    match (UNTAG);
    bfprintf (&stdout_buffer, "UNTAG\n");
    return true;
  case DESTROY:
    match (DESTROY);
    bfprintf (&stdout_buffer, "DESTROY\n");
    return true;
  case UNBIND:
    match (UNBIND);
    bfprintf (&stdout_buffer, "UNBIND\n");
    return true;
  case START:
    match (START);
    bfprintf (&stdout_buffer, "START\n");
    return true;
  case AID:
    match (AID);
    bfprintf (&stdout_buffer, "AID\n");
    return true;
  case BID:
    match (BID);
    bfprintf (&stdout_buffer, "BID\n");
    return true;
  case STRING:
    match (STRING);
    bfprintf (&stdout_buffer, "STRING\n");
    return true;
  case END:
  case RPAREN:
  case COMMA:
  case SEMICOLON:
  case PIPE:
    return false;
  }

  return false;
}

static bool
expression1b (void)
{
  bfprintf (&stdout_buffer, "expression1b\n");
  switch (current_token->type) {
  case PIPE:
    bfprintf (&stdout_buffer, "PIPE\n");
    match (PIPE);
    if (!expression0 ()) {
      return false;
    }
    return expression1b ();
  case END:
  case SEMICOLON:
  case RPAREN:
    /* lambda */
    return true;

  case LPAREN:
  case COMMA:
  case CREATE:
  case LOOKUP:
  case BIND:
  case TAG:
  case UNTAG:
  case DESTROY:
  case UNBIND:
  case START:
  case AID:
  case BID:
  case STRING:
    return false;
  }

  return false;
}

static bool
expression1 (void)
{
  bfprintf (&stdout_buffer, "expression1\n");
  switch (current_token->type) {
  case LPAREN:
  case CREATE:
  case LOOKUP:
  case BIND:
  case TAG:
  case UNTAG:
  case DESTROY:
  case UNBIND:
  case START:
  case AID:
  case BID:
  case STRING:
    if (!expression0 ()) {
      return false;
    }
    if (!expression1b ()) {
      return false;
    }
    return true;
  case END:
  case RPAREN:
  case COMMA:
  case SEMICOLON:
  case PIPE:
    return false;
  }

  return false;
}

static bool
trailer2 (void)
{
  bfprintf (&stdout_buffer, "trailer2\n");
  switch (current_token->type) {
  case END:
  case SEMICOLON:
    return trailer1 ();
  case LPAREN:
  case CREATE:
  case LOOKUP:
  case BIND:
  case TAG:
  case UNTAG:
  case DESTROY:
  case UNBIND:
  case START:
  case AID:
  case BID:
  case STRING:
    if (!expression1 ()) {
      return false;
    }
    return trailer1 ();
  case RPAREN:
  case COMMA:
  case PIPE:
    return false;
  }

  return false;
}

static bool
trailer1 (void)
{
  bfprintf (&stdout_buffer, "trailer1\n");
  switch (current_token->type) {
  case END:
    /* lambda */
    return true;
  case SEMICOLON:
    match (SEMICOLON);
    return trailer2 ();
  case LPAREN:
  case RPAREN:
  case COMMA:
  case PIPE:
  case CREATE:
  case LOOKUP:
  case BIND:
  case TAG:
  case UNTAG:
  case DESTROY:
  case UNBIND:
  case START:
  case AID:
  case BID:
  case STRING:
    return false;
  }

  return false;
}

static bool
program (void)
{
  bfprintf (&stdout_buffer, "program\n");
  switch (current_token->type) {
  case END:
  case SEMICOLON:
    if (!trailer1 ()) {
      return false;
    }
    if (!match (END)) {
      return false;
    }
    return true;
  case LPAREN:
  case CREATE:
  case LOOKUP:
  case BIND:
  case TAG:
  case UNTAG:
  case DESTROY:
  case UNBIND:
  case START:
  case AID:
  case BID:
  case STRING:
    if (!expression1 ()) {
      return false;
    }
    if (!trailer1 ()) {
      return false;
    }
    if (!match (END)) {
      return false;
    }
    return true;
  case RPAREN:
  case COMMA:
  case PIPE:
    /* Error. */
    return false;
  }

  return false;
}

/*
  End Parser Section
  ============================
*/

/*
  Begin Scanner Section
  =====================
*/

typedef enum {
  SCAN_START,
  SCAN_AID,
  SCAN_BID,
  SCAN_STRING,
  SCAN_COMMENT,
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
  scan_string_init ();
}

/* Tokens

   string - [a-zA-Z0-9_/-]+

   whitespace - [ \t]

   comment - #[^\n]*

   left paren - (

   right paren - )

   comma - ,

   semicolon - ;

   evaluation symbol - [;\n]

   automaton id - @[0-9]+

   binding id - &[0-9]+
 */
static bool
put (int c)
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
    case '(':
      push_token (LPAREN, 0, 0, 0);
      break;
    case ')':
      push_token (RPAREN, 0, 0, 0);
      break;
    case ',':
      push_token (COMMA, 0, 0, 0);
      break;
    case ';':
      push_token (SEMICOLON, 0, 0, 0);
      break;
    case '|':
      push_token (PIPE, 0, 0, 0);
      break;
    case '@':
      scan_state = SCAN_AID;
      break;
    case '&':
      scan_state = SCAN_BID;
      break;
    case '#':
      scan_state = SCAN_COMMENT;
      break;
    case ' ':
    case '\t':
    case '\n':
      /* Eat whitespace. */
      break;
    case -1:
      /* Used to force a finish if in the middle of a token. */
      break;
    default:
      return false;
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

    // Not a valid string character.
    {
      /* Terminate with null. */
      scan_string_append (0);
      char* string;
      size_t size;
      scan_string_steal (&string, &size);
      push_token (STRING, string, size, 0);
      scan_state = SCAN_START;
      // Recur.
      return put (c);
    }

  case SCAN_AID:
    if (c >= '0' && c <= '9') {
      /* Continue. */
      scan_string_append (c);
      break;
    }

    // Not a valid aid character.
    {
      /* Terminate with null. */
      scan_string_append (0);
      char* string;
      size_t size;
      scan_string_steal (&string, &size);
      push_token (AID, 0, 0, atoi (string));
      free (string);
      scan_state = SCAN_START;
      // Recur.
      return put (c);
    }
    
  case SCAN_BID:
    if (c >= '0' && c <= '9') {
      /* Continue. */
      scan_string_append (c);
      break;
    }

    // Not a valid bid character.
    {
      /* Terminate with null. */
      scan_string_append (0);
      char* string;
      size_t size;
      scan_string_steal (&string, &size);
      push_token (BID, 0, 0, atoi (string));
      free (string);
      scan_state = SCAN_START;
      // Recur.
      return put (c);
    }

  case SCAN_COMMENT:
    switch (c) {
    case -1:
      scan_state = SCAN_START;
      break;
    }
    break;
  }

  return true;
}

/*
  End Scanner Section
  ===================
*/

/*
  Begin Line Editing Section
  ==========================
*/

static char* line_str = 0;
static size_t line_size = 0;
static size_t line_capacity = 0;

/* Update the line of text. */
static void
line_update (const char* str,
	     size_t size)
{
  for (size_t idx = 0; idx != size; ++idx, ++str) {
    char c = *str;
    if (c >= ' ' && c < 127) {
      /* Printable character. */
      if (line_size == line_capacity) {
	line_capacity = 2 * line_capacity + 1;
	line_str = realloc (line_str, line_capacity);
      }
      line_str[line_size++] = c;
      if (buffer_file_put (&stdout_buffer, c) != 0) {
	exit ();
      }
    }
    else {
      /* Control character. */
      switch (c) {
      case '\n':
	if (buffer_file_put (&stdout_buffer, '\n') != 0) {
	  exit ();
	}
	/* Send the line to the scanner. */
	bool syntax_error = false;
	size_t i;
	for (i = 0; i != line_size; ++i) {
	  if (!put (line_str[i])) {
	    syntax_error = true;
	    break;
	  }
	}

	if (!syntax_error) {
	  /* Finish the current token. */
	  put (-1);
	  /* Push the end of input token. */
	  push_token (END, 0, 0, 0);
	  /* Try to evaluate a statement. */
	  current_token = token_list_head;
	  if (!program ()) {
	    bfprintf (&stdout_buffer, "parse error\n");
	  }
	  clean_tokens ();
	}
	else {
	  if (buffer_file_puts (&stdout_buffer, "syntax error near: ") != 0 ||
	      buffer_file_write (&stdout_buffer, line_str + i, line_size - i) != 0 ||
	      buffer_file_put (&stdout_buffer, '\n') != 0) {
	    exit ();
	  }
	}
	/* Reset. */
	line_size = 0;
	clean_tokens ();
	break;
      }
    }
  }
}

/*
  End Line Editing Section
  ========================
*/

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    stdout_bd = buffer_create (0);
    if (stdout_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&stdout_buffer, stdout_bd) != 0) {
      exit ();
    }

    scan_string_init ();
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

BEGIN_INPUT (NO_PARAMETER, START_NO, "start", "", start, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (!started) {
    started = true;
    if (buffer_file_puts (&stdout_buffer, PROMPT) != 0) {
      exit ();
    }
  }

  finish_input (bda, bdb);
}

BEGIN_INPUT (NO_PARAMETER, STDIN_NO, "stdin", "buffer_file_t", stdin, ano_t ano, int param, bd_t bda, bd_t bdb)
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

  line_update (str, size);

  finish_input (bda, bdb);
}

static bool
stdout_precondition (void)
{
  return buffer_file_size (&stdout_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, STDOUT_NO, "stdout", "buffer_file_t", stdout, ano_t ano, int param)
{
  initialize ();

  if (stdout_precondition ()) {
    buffer_file_truncate (&stdout_buffer);
    finish_output (true, stdout_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

void
do_schedule (void)
{
  if (stdout_precondition ()) {
    schedule (STDOUT_NO, 0);
  }
}
