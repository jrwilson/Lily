#include "shell.h"
#include <automaton.h>
#include <string.h>
#include <buffer_heap.h>
#include "terminal.h"
#include "keyboard.h"
#include <fifo_scheduler.h>
#include <dymem.h>
#include <string_buffer.h>

/*
  Shell
  =====
  A text-based shell for the Lily kernel.
  Users enter commands to create and manipulate a constellation of automata and bindings.
  Information is displayed using lists and menus.
  Ideally, a graphic approach would be used, however, lists and menus are an easy starting point.

  Local IDs vs. Global IDs
  ------------------------
  Automata (and possibly bindings) have global IDs that can be drawn from a large set.
  I believe it would be useful to remap the global IDs into a smaller set of local IDs.
  The user would manipulate automata and bindings using the local IDs.
  This should save them some typing and reduces mistakes because its certainly easier to remember a one or two digit number than it is a six or ten digit number.

 */

/*
  Model Section
  =============
  Data and functions for manipulating the constellation of automata.
*/

typedef struct automaton_struct automaton_t;
typedef struct action_struct action_t;
typedef struct binding_struct binding_t;

struct action_struct {
  char* name;			/* The action name.  Must be unique. */
  automaton_t* automaton;	/* The associated automaton. */
  binding_t* binding_next;	/* The bindings associated with this action. */
/*   ano_t ano;			/\* The action number. *\/ */
  action_t* next;
};

struct automaton_struct {
  /* aid_t aid;		/\* The global identifier for this automaton. *\/ */
  int lid;		/* The local identifier for this automaton. */
  action_t* inputs;	/* The list of input actions. */
  action_t* outputs;	/* The list of output actions. */
  automaton_t* next;
};

struct binding_struct {
  int lid;			/* The local identifier for this binding. */
  action_t* output_action;	/* The output action and parameter. */
  int output_parameter;
  binding_t* output_next;	/* The next binding related to an output action. */
  action_t* input_action;	/* The input action and parameter. */
  int input_parameter;
  binding_t* input_next;	/* The next binding related to an input action. */
  /* bid_t bid;			/\* The global identifier for this binding. *\/ */
  binding_t* next;
};

static automaton_t* automaton_list_head = 0;
static binding_t* binding_list_head = 0;

static action_t*
create_action (const char* name,
	       automaton_t* automaton)
{
  action_t* action = malloc (sizeof (action_t));
  memset (action, 0, sizeof (action_t));
  size_t size = strlen (name) + 1;
  action->name = malloc (size);
  memcpy (action->name, name, size);
  action->automaton = automaton;
  return action;
}

static void
destroy_action (action_t* action)
{
  free (action->name);
  free (action);
}

static void
add_output_binding (action_t* action,
		    binding_t* binding)
{
  binding->output_next = action->binding_next;
  action->binding_next = binding;
}

static void
add_input_binding (action_t* action,
		   binding_t* binding)
{
  binding->input_next = action->binding_next;
  action->binding_next = binding;
}

static automaton_t*
create_automaton (int lid)
{
  automaton_t* automaton = malloc (sizeof (automaton_t));
  memset (automaton, 0, sizeof (automaton_t));
  automaton->lid = lid;
  return automaton;
}

static void
destroy_automaton (automaton_t* automaton)
{
  while (automaton->inputs != 0) {
    action_t* tmp = automaton->inputs;
    automaton->inputs = tmp->next;
    destroy_action (tmp);
  }
  while (automaton->outputs != 0) {
    action_t* tmp = automaton->outputs;
    automaton->outputs = tmp->next;
    destroy_action (tmp);
  }
  free (automaton);
}

static action_t*
find_input (automaton_t* automaton,
	    const char* name)
{
  action_t* action;
  for (action = automaton->inputs; action != 0; action = action->next) {
    if (strcmp (action->name, name) == 0) {
      break;
    }
  }
  return action;
}

static void
add_input (automaton_t* automaton,
	   const char* name)
{
  if (find_input (automaton, name) == 0) {
    action_t* action = create_action (name, automaton);
    action->next = automaton->inputs;
    automaton->inputs = action;
  }
}

static action_t*
find_output (automaton_t* automaton,
	     const char* name)
{
  action_t* action;
  for (action = automaton->outputs; action != 0; action = action->next) {
    if (strcmp (action->name, name) == 0) {
      break;
    }
  }
  return action;
}

static void
add_output (automaton_t* automaton,
	    const char* name)
{
  if (find_output (automaton, name) == 0) {
    action_t* action = create_action (name, automaton);
    action->next = automaton->outputs;
    automaton->outputs = action;
  }
}

static automaton_t*
find_automaton (int lid)
{
  automaton_t* automaton;
  for (automaton = automaton_list_head; automaton != 0; automaton = automaton->next) {
    if (automaton->lid == lid) {
      break;
    }
  }
  return automaton;
}

static binding_t*
create_binding (int lid,
		action_t* output_action,
		int output_parameter,
		action_t* input_action,
		int input_parameter)
{
  binding_t* binding = malloc (sizeof (binding_t));
  memset (binding, 0, sizeof (binding_t));
  binding->lid = lid;
  binding->output_action = output_action;
  binding->output_parameter = output_parameter;
  binding->input_action = input_action;
  binding->input_parameter = input_parameter;
  return binding;
}

/*
  Output Section
  ==============
  Data and functions for drawing on the terminal.
*/

/* The output buffer. */
#define STDOUT_BUFFER_SIZE 4000
static string_buffer_t stdout;

static void
clear_screen (void)
{
  string_buffer_puts (&stdout, ESC_S CSI_S ERASE_ALL_S ED_S);
}

static void
show_prompt (void)
{
  string_buffer_puts (&stdout, "> ");
}

static void
list_automata (void)
{
  string_buffer_puts (&stdout, "automata:\n");
  
  if (automaton_list_head != 0) {
    for (automaton_t* ptr = automaton_list_head; ptr != 0; ptr = ptr->next) {
      sbprintf (&stdout, "  %d\n", ptr->lid);
    }
  }
  else {
    string_buffer_puts (&stdout, "  (empty)\n");
  }
}

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

/*
  Input Section
  =============
  Data and functions for reading input and parsing commands.
*/

/* The line buffer contains data as it arrives from stdin. */
static char* line_buffer;
static size_t line_buffer_size;
static size_t line_buffer_capacity;

static void
line_buffer_putc (const char c)
{
  if (line_buffer_size == line_buffer_capacity) {
    line_buffer_capacity += 1;
    line_buffer_capacity *= 2;
    line_buffer = realloc (line_buffer, line_buffer_capacity);
  }
  line_buffer[line_buffer_size++] = c;
}

/* A list of tokens. */
typedef struct token_list_item_struct token_list_item_t;
struct token_list_item_struct {
  char* token;
  token_list_item_t* next;
};

static token_list_item_t* token_list_head = 0;
static token_list_item_t** token_list_tail = &token_list_head;
static token_list_item_t* current_token = 0;

static void
token_list_append (char* tok)
{
  token_list_item_t* tli = malloc (sizeof (token_list_item_t));
  tli->token = tok;
  tli->next = 0;

  *token_list_tail = tli;
  token_list_tail = &tli->next;
}

static void
token_list_reset (void)
{
  while (token_list_head != 0) {
    token_list_item_t* item = token_list_head;
    token_list_head = item->next;
    free (item);
  }

  token_list_tail = &token_list_head;
}

static void
tokenize (void)
{
  char* abs_begin = line_buffer;
  char* abs_end = line_buffer + line_buffer_size;
  
  for (char* ptr = abs_begin; ptr != abs_end; ++ptr) {
    /* Replace delimiters with null. */
    switch (*ptr) {
    case ' ':
      *ptr = 0;
      break;
    }
  }

  for (char* begin = abs_begin; begin != abs_end; ++begin) {
    if (*begin != 0) {
      /* Found the beginning of a token. */
      token_list_append (begin);
      ++begin;
      while (*begin != 0) {
	++begin;
      }
    }
  }
}

/* Functions for a recursive descent parser. */

static void
advance (void)
{
  if (current_token != 0) {
    current_token = current_token->next;
  }
}

static bool
accept (const char* str)
{

  if ((current_token != 0) && (strcmp (current_token->token, str) == 0)) {
    advance ();
    return true;
  }
  else {
    return false;
  }
}

static const char*
expect (const char* err_msg)
{
  if (current_token == 0) {
    string_buffer_puts (&stdout, err_msg);
    return 0;
  }
  else {
    const char* retval = current_token->token;
    advance ();
    return retval;
  }
}

#define BIND_USAGE "usage: bind ID OUTPUT PARAMETER ID INPUT PARAMETER\n"
#define DESTROY_USAGE "usage: destroy ID\n"

static void
list_command (void)
{
  list_automata ();
}

static void
detail_command (void)
{
  if (current_token == 0) {
    string_buffer_puts (&stdout, "usage: detail ID\n");
    return;
  }
  
  const char* t = current_token->token;
  advance ();

  int lid = atoi (t);

  automaton_t* automaton;
  for (automaton = automaton_list_head; automaton != 0; automaton = automaton->next) {
    if (automaton->lid == lid) {
      break;
    }
  }

  if (automaton != 0) {
    sbprintf (&stdout, "automaton %d detail:\n", lid);

    sbprintf (&stdout, "  inputs:\n");
    if (automaton->inputs != 0) {
      for (action_t* action = automaton->inputs; action != 0; action = action->next) {
	sbprintf (&stdout, "    %s\n", action->name);
	for (binding_t* binding = action->binding_next; binding != 0; binding = binding->input_next) {
	  sbprintf (&stdout, "      (%d, %s, %d) -> *(%d, %s, %d)\n", binding->output_action->automaton->lid, binding->output_action->name, binding->output_parameter, binding->input_action->automaton->lid, binding->input_action->name, binding->input_parameter);
	}
      }
    }
    else {
      sbprintf (&stdout, "    (empty)\n");
    }

    sbprintf (&stdout, "  outputs:\n");
    if (automaton->outputs != 0) {
      for (action_t* action = automaton->outputs; action != 0; action = action->next) {
	sbprintf (&stdout, "    %s\n", action->name);
	for (binding_t* binding = action->binding_next; binding != 0; binding = binding->output_next) {
	  sbprintf (&stdout, "      *(%d, %s, %d) -> (%d, %s, %d)\n", binding->output_action->automaton->lid, binding->output_action->name, binding->output_parameter, binding->input_action->automaton->lid, binding->input_action->name, binding->input_parameter);
	}
      }
    }
    else {
      sbprintf (&stdout, "    (empty)\n");
    }
  }
  else {
    sbprintf (&stdout, "automaton %s does not exist\n", t);
  }
}

static void
create_command (void)
{
  /* Find a local id. */
  int lid = 1;
  automaton_t** ptr;
  for (ptr = &automaton_list_head; *ptr != 0; ptr = &((*ptr)->next)) {
    if ((*ptr)->lid == lid) {
      ++lid;
    }
    else {
      break;
    }
  }

  automaton_t* automaton = create_automaton (lid);
  /* TODO:  Remove these. */
  add_input (automaton, "stdin");
  add_output (automaton, "stdout");
  add_output (automaton, "stderr");

  /* Insert in sorted order. */
  automaton->next = *ptr;
  *ptr = automaton;

  sbprintf (&stdout, "id = %d\n", lid);
}

static void
bind_command (void)
{
  const char* output_automaton_str = expect (BIND_USAGE);
  if (output_automaton_str == 0) {
    return;
  }

  const char* output_action_str = expect (BIND_USAGE);
  if (output_action_str == 0) {
    return;
  }

  const char* output_parameter_str = expect (BIND_USAGE);
  if (output_parameter_str == 0) {
    return;
  }

  const char* input_automaton_str = expect (BIND_USAGE);
  if (input_automaton_str == 0) {
    return;
  }

  const char* input_action_str = expect (BIND_USAGE);
  if (input_action_str == 0) {
    return;
  }

  const char* input_parameter_str = expect (BIND_USAGE);
  if (input_parameter_str == 0) {
    return;
  }

  automaton_t* output_automaton = find_automaton (atoi (output_automaton_str));
  if (output_automaton == 0) {
    sbprintf (&stdout, "automaton %s does not exist\n", output_automaton_str);
    return;
  }

  action_t* output_action = find_output (output_automaton, output_action_str);
  if (output_action == 0) {
    sbprintf (&stdout, "action %s does not exist\n", output_action_str);
    return;
  }
    
  int output_parameter = atoi (output_parameter_str);

  automaton_t* input_automaton = find_automaton (atoi (input_automaton_str));
  if (input_automaton == 0) {
    sbprintf (&stdout, "automaton %s does not exist\n", input_automaton_str);
    return;
  }

  action_t* input_action = find_input (input_automaton, input_action_str);
  if (input_action == 0) {
    sbprintf (&stdout, "action %s does not exist\n", input_action_str);
    return;
  }
    
  int input_parameter = atoi (input_parameter_str);

  /* Find a local id. */
  int lid = 1;
  binding_t** ptr;
  for (ptr = &binding_list_head; *ptr != 0; ptr = &((*ptr)->next)) {
    if ((*ptr)->lid == lid) {
      ++lid;
    }
    else {
      break;
    }
  }

  binding_t* binding = create_binding (lid, output_action, output_parameter, input_action, input_parameter);
  add_output_binding (output_action, binding);
  add_input_binding (input_action, binding);

  /* Insert in sorted order. */
  binding->next = *ptr;
  *ptr = binding;

  sbprintf (&stdout, "id = %d\n", lid);
}

static void
unbind_command (void)
{
  sbprintf (&stdout, "unbind\n");
}

static void
destroy_command (void)
{
  const char* t = expect (DESTROY_USAGE);
  if (t == 0) {
    return;
  }

  int lid = atoi (t);

  automaton_t** ptr;
  for (ptr = &automaton_list_head; *ptr != 0; ptr = &(*ptr)->next) {
    if ((*ptr)->lid == lid) {
      break;
    }
  }

  if (*ptr != 0) {
    automaton_t* temp = *ptr;
    *ptr = temp->next;
    destroy_automaton (temp);
  }
  else {
    sbprintf (&stdout, "automaton %s does not exist\n", t);
  }
}

static void
expression (void)
{
  if (current_token == 0) {
    /* An expression can be empty. */
  }
  else if (accept ("list")) {
    list_command ();
  }
  else if (accept ("detail")) {
    detail_command ();
  }
  else if (accept ("create")) {
    create_command ();
  }
  else if (accept ("bind")) {
    bind_command ();
  }
  else if (accept ("unbind")) {
    unbind_command ();
  }
  else if (accept ("destroy")) {
    destroy_command ();
  }
  else {
    sbprintf (&stdout, "unknown command: %s\n", current_token->token);
  }
}

static void
process_input (bd_t bd,
	       void* ptr,
	       size_t buffer_size)
{
  if (ptr == 0) {
    /* Either no data or we can't map it. */
    return;
  }
  
  string_buffer_t sb;
  if (!string_buffer_parse (&sb, bd, ptr, buffer_size)) {
    /* Bad buffer. */
    return;
  }

  const char* string = string_buffer_data (&sb);
  const size_t size = string_buffer_size (&sb);

  for (size_t idx = 0; idx != size; ++idx) {
    const char c = string[idx];

    /* Echo to stdout. */
    string_buffer_putc (&stdout, c);

    /* TODO: Interpret control characters. */
    switch (c) {
    case '\n':
      /* Append a null. */
      line_buffer_putc (0);
      /* Convert the line buffer into tokens. */
      tokenize ();
      current_token = token_list_head;

      /* Evaluate. */
      expression ();

      /* Reset everything. */
      token_list_reset ();
      line_buffer_size = 0;

      show_prompt ();

      break;
    default:
      line_buffer_putc (c);
      break;
    }
  }
}

static size_t shell_stdout_binding_count = 1;

static void
schedule (void);

void
init (int param,
      bd_t bd,
      void* ptr,
      size_t buffer_size)
{
  string_buffer_init (&stdout, STDOUT_BUFFER_SIZE);

  clear_screen ();
  show_prompt ();
  
  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);

void
shell_stdin (int param,
	     bd_t bd,
	     void* ptr,
	     size_t size)
{
  process_input (bd, ptr, size);

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, SHELL_STDIN, shell_stdin);

static bool
shell_stdout_precondition (void)
{
  return string_buffer_size (&stdout) != 0 && shell_stdout_binding_count != 0;
}

void
shell_stdout (int param,
	      size_t bc)
{
  scheduler_remove (SHELL_STDOUT, param);
  shell_stdout_binding_count = bc;

  if (shell_stdout_precondition ()) {
    bd_t bd = string_buffer_bd (&stdout);
    string_buffer_init (&stdout, STDOUT_BUFFER_SIZE);

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, SHELL_STDOUT, shell_stdout);

static void
schedule (void)
{
  if (shell_stdout_precondition ()) {
    scheduler_add (SHELL_STDOUT, 0);
  }
}
