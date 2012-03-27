#include <automaton.h>
#include <buffer_file.h>
#include <dymem.h>
#include <string.h>
#include <description.h>
#include "vfs_msg.h"
#include <callback_queue.h>
#include "argv.h"

/* TODO:  Improve the error handling. */

/* TODO:  Replace bfprintf with lightweight alternatives. put puts, etc. */

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
#define START_NO 2
#define STDIN_NO 3
#define PROCESS_LINE_NO 4
#define STDOUT_NO 5
#define VFS_RESPONSE_NO 6
#define VFS_REQUEST_NO 7

/* Initialization flag. */
static bool initialized = false;

static bd_t stdout_bd = -1;
static buffer_file_t stdout_buffer;

#define VFS_NAME "vfs"
static bd_t vfs_request_bda = -1;
static bd_t vfs_request_bdb = -1;
static vfs_request_queue_t vfs_request_queue;
static callback_queue_t vfs_response_queue;
static bool process_hold = false;

typedef struct label label_t;
typedef struct label_item label_item_t;
typedef struct automaton automaton_t;
typedef struct automaton_item automaton_item_t;
typedef struct binding binding_t;
typedef struct binding_item binding_item_t;

struct label {
  char* string;
  automaton_item_t* automata_head;
  binding_item_t* bindings_head;
  label_t* next;
};

struct label_item {
  label_t* label;
  label_item_t* next;
};

struct automaton {
  char* name;
  aid_t aid;
  char* path;
  label_item_t* labels_head;
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
  label_item_t* labels_head;
  binding_t* next;
};

struct binding_item {
  binding_t* binding;
  binding_item_t* next;
};

static label_t* labels_head = 0;
static automaton_t* automata_head = 0;
static binding_t* bindings_head = 0;

static label_t*
find_label (const char* string)
{
  label_t* label;
  for (label = labels_head; label != 0; label = label->next) {
    if (strcmp (label->string, string) == 0) {
      break;
    }
  }
  return label;
}

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

  bfprintf (&stdout_buffer, "TODO: subscribe to automaton\n");
  
  automaton->next = automata_head;
  automata_head = automaton;
  return automaton;
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

static automaton_t*
find_automaton_aid (aid_t aid)
{
  automaton_t* automaton;
  for (automaton = automata_head; automaton != 0; automaton = automaton->next) {
    if (automaton->aid == aid) {
      break;
    }
  }
  return automaton;
}

static void
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

  bfprintf (&stdout_buffer, "TODO: subscribe to binding\n");

  binding->next = bindings_head;
  bindings_head = binding;
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

static void
string_push_front (char* str,
		   size_t size)
{
  string_t* s = malloc (sizeof (string_t));
  s->str = str;
  s->size = size;
  s->next = string_head;
  string_head = s;

  if (s->next == 0) {
    string_tail = &s->next;
  }
}

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

   string - [a-zA-Z0-9_/-=*]+

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
	(c == '*')) {
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
	(c == '*')) {
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
      bfprintf (&stdout_buffer, "error near: %s\n", string + idx);
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

typedef struct start_item start_item_t;
struct start_item {
  aid_t aid;
  start_item_t* next;
};

static start_item_t* start_head = 0;
static start_item_t** start_tail = &start_head;

static bool
start_queue_empty (void)
{
  return start_head == 0;
}

static aid_t
start_queue_front (void)
{
  return start_head->aid;
}

static void
start_queue_push (aid_t aid)
{
  start_item_t* item = malloc (sizeof (start_item_t));
  memset (item, 0, sizeof (start_item_t));
  item->aid = aid;
  *start_tail = item;
  start_tail = &item->next;
}

static void
start_queue_pop (void)
{
  start_item_t* item = start_head;
  start_head = item->next;
  if (start_head == 0) {
    start_tail = &start_head;
  }

  free (item);
}

static const char* create_name = 0;
static bool create_retain_privilege = false;
static const char* create_register_name = 0;
static size_t create_register_name_size = 0;
static const char* create_path = 0;

static void
create_callback (void* data,
		 bd_t bda,
		 bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) != 0) {
    exit ();
  }

  if (error == VFS_SUCCESS) {
    bfprintf (&stdout_buffer, "TODO:  argv\n");
    aid_t aid = create (bdb, size, -1, -1, create_register_name, create_register_name_size, create_retain_privilege);
    if (aid != -1) {
      bfprintf (&stdout_buffer, "TODO:  Subscribe to created automaton\n");

      /* Add to the list of automata we know about. */
      create_automaton (create_name, aid, create_path);
      
      bfprintf (&stdout_buffer, "-> %s = %d\n", scan_strings[0], aid);
    }
    else {
      bfprintf (&stdout_buffer, "TODO:  create create failed\n");
    }
  }
  else {
    bfprintf (&stdout_buffer, "TODO:  create couldn't read file\n");
  }
}

static void
create_usage (void)
{
  bfprintf (&stdout_buffer, "usage: NAME = create [-p -n NAME] create PATH [OPTIONS...]\n");
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

    if (find_label (create_name) != 0) {
      bfprintf (&stdout_buffer, "error: %s is already a label\n", create_name);
      return true;
    }

    if (find_automaton (create_name) != 0) {
      bfprintf (&stdout_buffer, "error: name %s is taken\n", create_name);
      return true;
    }

    size_t idx = 3;
    
    /* Parse the options. */
    create_retain_privilege = false;
    create_register_name = 0;
    create_register_name_size = 0;
    
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
	create_register_name_size = strlen (create_register_name) + 1;
	++idx;
      }
      
      break;
    }

    if (idx >= scan_strings_size) {
      create_usage ();
      return -1;
    }
    create_path = scan_strings[idx++];

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
  bfprintf (&stdout_buffer, "usage: bind [-o OPARAM -i IPARAM] OAID OACTION IAID IACTION\n");
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
	  return -1;
	}
	
	if (strcmp (scan_strings[idx], "-o") == 0) {
	  ++idx;
	  if (idx >= scan_strings_size) {
	    bind_usage ();
	    return -1;
	  }
	  
	  output_parameter = atoi (scan_strings[idx]);
	  ++idx;
	  continue;
	}
	
	if (strcmp (scan_strings[idx], "-i") == 0) {
	  ++idx;
	  if (idx >= scan_strings_size) {
	    bind_usage ();
	    return -1;
	  }
	  
	  input_parameter = atoi (scan_strings[idx]);
	  ++idx;
	}
	
	break;
      }
      
      if (idx + 4 != scan_strings_size) {
	bind_usage ();
	return -1;
      }
      
      automaton_t* output_automaton = find_automaton (scan_strings[idx]);
      if (output_automaton == 0) {
	bfprintf (&stdout_buffer, "error: %s does not refer to a known automaton\n", scan_strings[idx]);
	return -1;
      }
      ++idx;
      
      const char* output_action_name = scan_strings[idx++];
      
      automaton_t* input_automaton = find_automaton (scan_strings[idx]);
      if (input_automaton == 0) {
	bfprintf (&stdout_buffer, "error: %s does not refer to a known automaton\n", scan_strings[idx]);
	return -1;
      }
      ++idx;
      
      const char* input_action_name = scan_strings[idx++];
      
      /* Describe the output and input automaton. */
      description_t output_description;
      description_t input_description;
      if (description_init (&output_description, output_automaton->aid) != 0) {
	bfprintf (&stdout_buffer, "TODO: Could not describe output\n");
	return -1;
      }
      if (description_init (&input_description, input_automaton->aid) != 0) {
	bfprintf (&stdout_buffer, "TODO: Could not describe input\n");
	return -1;
      }
      
      /* Look up the actions. */
      const ano_t output_action = description_name_to_number (&output_description, output_action_name, strlen (output_action_name) + 1);
      const ano_t input_action = description_name_to_number (&input_description, input_action_name, strlen (input_action_name) + 1);
      
      description_fini (&output_description);
      description_fini (&input_description);
      
      if (output_action == -1) {
	bfprintf (&stdout_buffer, "TODO: Output action does not exist\n");
	return -1;
      }
      
      if (input_action == -1) {
	bfprintf (&stdout_buffer, "TODO: Input action does not exist\n");
	return -1;
      }
      
      bfprintf (&stdout_buffer, "TODO: Correct the parameters\n");
      
      bid_t bid = bind (output_automaton->aid, output_action, output_parameter, input_automaton->aid, input_action, input_parameter);
      if (bid == -1) {
	bfprintf (&stdout_buffer, "TODO: Bind failed\n");
	return -1;
      }
      
      create_binding (bid, output_automaton, output_action, output_action_name, output_parameter, input_automaton, input_action, input_action_name, input_parameter);
      
      bfprintf (&stdout_buffer, "-> %d\n", bid);
      return true;
    }
  }
  return false;
}

static void
lookup_usage (void)
{
  bfprintf (&stdout_buffer, "usage: NAME = lookup NAME\n");
}

static bool
lookup_ (void)
{
  if (scan_strings_size >= 1) {
    if (strcmp ("lookup", scan_strings[0]) == 0) {
      lookup_usage ();
      return true;
    }
  }
  if (scan_strings_size >= 3 && 
      strcmp ("=", scan_strings[1]) == 0 &&
      strcmp ("lookup", scan_strings[2]) == 0) {
    if (scan_strings_size == 4) {
      /* Automaton names and labels are disjoint. */
      if (find_label (scan_strings[0]) != 0) {
	bfprintf (&stdout_buffer, "error: %s is already a label\n", scan_strings[0]);
	return true;
      }
      
      if (find_automaton (scan_strings[0]) != 0) {
	bfprintf (&stdout_buffer, "error: name %s is taken\n", scan_strings[0]);
	return true;
      }
      
      /* Perform the lookup. */
      aid_t aid = lookup (scan_strings[3], strlen (scan_strings[3]) + 1);
      if (aid == -1) {
	bfprintf (&stdout_buffer, "no automaton registered under %s\n", scan_strings[3]);
	return true;
      }
      
      automaton_t* a;
      if ((a = find_automaton_aid (aid)) != 0) {
	bfprintf (&stdout_buffer, "error: automaton already exists with name %s\n", a->name);
	return true;
      }
      
      /* Add to the list of automata we know about. */
      create_automaton (scan_strings[0], aid, scan_strings[3]);
      
      bfprintf (&stdout_buffer, "-> %s = %d\n", scan_strings[0], aid);
      return true;
    }
    else {
      lookup_usage ();
      return true;
    }
  }
  return false;
}

static bool
describe_ (void)
{
  if (scan_strings_size >= 1 && strcmp (scan_strings[0], "describe") == 0) {
    if (scan_strings_size == 1) {
      bfprintf (&stdout_buffer, "usage: describe NAME...\n");
      return true;
    }

    action_desc_t* actions = 0;

    for (size_t idx = 1; idx != scan_strings_size; ++idx) {
      bfprintf (&stdout_buffer, "%s:\n", scan_strings[idx]);
      automaton_t* automaton = find_automaton (scan_strings[idx]);
      if (automaton == 0) {
	bfprintf (&stdout_buffer, "\t (name not recognized)\n");
	continue;
      }

      description_t description;
      if (description_init (&description, automaton->aid) != 0) {
	bfprintf (&stdout_buffer, "\t(no description)\n");
	continue;
      }

      size_t action_count = description_action_count (&description);
      if (action_count == -1) {
	bfprintf (&stdout_buffer, "\t(bad description)\n");
	description_fini (&description);
	continue;
      }

      if (action_count == 0) {
	bfprintf (&stdout_buffer, "\t(empty)\n");
	description_fini (&description);
	continue;
      }

      actions = realloc (actions, action_count * sizeof (action_desc_t));

      if (description_read (&description, actions) != 0) {
	bfprintf (&stdout_buffer, "\t(bad description)\n");
	description_fini (&description);
	continue;
      }

      for (size_t action = 0; action != action_count; ++action) {
	switch (actions[action].type) {
	case LILY_ACTION_INPUT:
	  bfprintf (&stdout_buffer, "\t IN ");
	  break;
	case LILY_ACTION_OUTPUT:
	  bfprintf (&stdout_buffer, "\tOUT ");
	  break;
	case LILY_ACTION_INTERNAL:
	  bfprintf (&stdout_buffer, "\tINT ");
	  break;
	case LILY_ACTION_SYSTEM_INPUT:
	  bfprintf (&stdout_buffer, "\tSYS ");
	  break;
	default:
	  bfprintf (&stdout_buffer, "\t??? ");
	  break;
	}

	switch (actions[action].parameter_mode) {
	case NO_PARAMETER:
	  bfprintf (&stdout_buffer, "  ");
	  break;
	case PARAMETER:
	  bfprintf (&stdout_buffer, "* ");
	  break;
	case AUTO_PARAMETER:
	  bfprintf (&stdout_buffer, "@ ");
	  break;
	default:
	  bfprintf (&stdout_buffer, "? ");
	  break;
	}

	bfprintf (&stdout_buffer, "%d\t\t%s\t\t%s\n", actions[action].number, actions[action].name, actions[action].description);
      }

      description_fini (&description);
    }

    free (actions);

    return true;
  }
  return false;
}

static bool
show_ (void)
{
  if (scan_strings_size >= 1) {
    if (strcmp ("show", scan_strings[0]) == 0) {
      if (scan_strings_size == 1) {

	bfprintf (&stdout_buffer, "automata:\n");
	if (automata_head != 0) {
	  for (automaton_t* automaton = automata_head; automaton != 0; automaton = automaton->next) {
	    bfprintf (&stdout_buffer, "\t%d:\t%s\t%s\n", automaton->aid, automaton->name, automaton->path);
	  }
	}
	else {
	  bfprintf (&stdout_buffer, "\t[empty]\n");
	}

	bfprintf (&stdout_buffer, "bindings:\n");
	if (bindings_head != 0) {
	  for (binding_t* binding = bindings_head; binding != 0; binding = binding->next) {
	    bfprintf (&stdout_buffer, "\t%d: (%s, %s, %d) -> (%s, %s, %d)\n", binding->bid, binding->output_automaton->name, binding->output_action_name, binding->output_parameter, binding->input_automaton->name, binding->input_action_name, binding->input_parameter);
	  }
	}
	else {
	  bfprintf (&stdout_buffer, "\t[empty]\n");
	}

	return true;
      }
      else {
	bfprintf (&stdout_buffer, "usage: show\n");
	return true;
      }
    }
  }
  return false;
}

static bool
start_ (void)
{
  if (scan_strings_size >= 1 && strcmp (scan_strings[0], "start") == 0) {
    if (scan_strings_size == 1) {
      bfprintf (&stdout_buffer, "usage: start ID...\n");
      return true;
    }

    for (size_t idx = 1; idx != scan_strings_size; ++idx) {
      label_t* label = find_label (scan_strings[idx]);
      if (label != 0) {
	bfprintf (&stdout_buffer, "TODO start label %s\n", scan_strings[idx]);
	continue;
      }
      automaton_t* automaton = find_automaton (scan_strings[idx]);
      if (automaton != 0) {
	/* Bind to its start action if necessary. */
	bfprintf (&stdout_buffer, "TODO bind to start\n");
	
	/* Add to the start queue. */
	start_queue_push (automaton->aid);
	continue;
      }

      bfprintf (&stdout_buffer, "warning: label or name %s does not exist\n", scan_strings[idx]);
    }

    return true;
  }
  return false;
}

static bool
error_ (void)
{
  if (scan_strings_size != 0) {
    /* Catch all. */
    bfprintf (&stdout_buffer, "error: unknown command %s\n", scan_strings[0]);
  }
  return true;
}

typedef bool (*dispatch_func_t) (void);

static dispatch_func_t dispatch[] = {
  create_,
  bind_,
  lookup_,
  describe_,
  show_,
  start_,
  /* Catch errors. */
  error_,
  0
};

/*
  End Dispatch Section
  ====================
*/

static void
readscript_callback (void* data,
		     bd_t bda,
		     bd_t bdb)
{
  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) != 0) {
    exit ();
  }

  if (error != VFS_SUCCESS) {
    exit ();
  }

  const char* str = buffer_map (bdb);
  if (str == 0) {
    exit ();
  }
  
  char* str_copy = malloc (size + 1);
  memcpy (str_copy, str, size);
  /* Terminate with newline. */
  str_copy[size] = '\n';
  string_push_front (str_copy, size + 1);
  buffer_unmap (bdb);
  process_hold = false;
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    aid_t aid = getaid ();
    bd_t bda = getinita ();
    bd_t bdb = getinitb ();

    create_automaton ("this", aid, "this");

    stdout_bd = buffer_create (0);
    if (stdout_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&stdout_buffer, stdout_bd) != 0) {
      exit ();
    }

    vfs_request_bda = buffer_create (0);
    vfs_request_bdb = buffer_create (0);
    if (vfs_request_bda == -1 ||
    	vfs_request_bdb == -1) {
      exit ();
    }
    vfs_request_queue_init (&vfs_request_queue);
    callback_queue_init (&vfs_response_queue);

    /* Bind to the vfs. */
    aid_t vfs_aid = lookup (VFS_NAME, strlen (VFS_NAME) + 1);
    if (vfs_aid == -1) {
      exit ();
    }
    
    description_t vfs_description;
    if (description_init (&vfs_description, vfs_aid) != 0) {
      exit ();
    }
    const ano_t vfs_request = description_name_to_number (&vfs_description, VFS_REQUEST_NAME, strlen (VFS_REQUEST_NAME) + 1);
    const ano_t vfs_response = description_name_to_number (&vfs_description, VFS_RESPONSE_NAME, strlen (VFS_RESPONSE_NAME) + 1);
    description_fini (&vfs_description);
    
    if (vfs_request == -1 ||
    	vfs_response == -1) {
      /* VFS does not appear to be a vfs. */
      exit ();
    }
    
    /* We bind the response first so they don't get lost. */
    if (bind (vfs_aid, vfs_response, 0, aid, VFS_RESPONSE_NO, 0) == -1 ||
    	bind (aid, VFS_REQUEST_NO, 0, vfs_aid, vfs_request, 0) == -1) {
      exit ();
    }
    
    argv_t argv;
    size_t argc;
    if (argv_initr (&argv, bda, bdb, &argc) == 0) {
      if (argc >= 2) {
    	/* Request the script. */
    	const char* filename;
    	size_t size;
    	if (argv_arg (&argv, 1, (const void**)&filename, &size) != 0) {
	  exit ();
    	}

    	vfs_request_queue_push_readfile (&vfs_request_queue, filename);
	process_hold = true;
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

  char* str_copy = malloc (size);
  memcpy (str_copy, str, size);

  string_push_back (str_copy, size);

  finish_input (bda, bdb);
}

static bool
process_line_precondition (void)
{
  return !process_hold && !string_empty () && start_queue_empty () && callback_queue_empty (&vfs_response_queue);
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
	if (buffer_file_put (&stdout_buffer, c) != 0) {
	  exit ();
	}
      }
      else {
	/* Control character. */
	switch (c) {
	case '\n':
	  /* Terminate. */
	  line_append (0);
	  if (buffer_file_put (&stdout_buffer, '\n') != 0) {
	    exit ();
	  }

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

BEGIN_OUTPUT (AUTO_PARAMETER, START_NO, "start", "", start, ano_t ano, aid_t aid)
{
  initialize ();

  if (!start_queue_empty () && aid == start_queue_front ()) {
    start_queue_pop ();
    finish_output (true, -1, -1);
  }
  finish_output (false, -1, -1);
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
      exit ();
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

void
do_schedule (void)
{
  if (process_line_precondition ()) {
    schedule (PROCESS_LINE_NO, 0);
  }
  if (!start_queue_empty ()) {
    schedule (START_NO, start_queue_front ());
  }
  if (stdout_precondition ()) {
    schedule (STDOUT_NO, 0);
  }
  if (vfs_request_precondition ()) {
    schedule (VFS_REQUEST_NO, 0);
  }
}
