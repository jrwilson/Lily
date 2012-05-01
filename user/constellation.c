#include "constellation.h"

#include <dymem.h>
#include <string.h>
#include <description.h>

/* TODO:  Reference counting. */

typedef enum {
  MANAGED,
  UNMANAGED,
} automaton_type_t;

typedef void (*event_handler_t) (void* arg);

typedef struct event_handler_item event_handler_item_t;
struct event_handler_item {
  event_handler_t event_handler;
  void* arg;
  event_handler_item_t* next;
};
  
struct automaton {
  automaton_type_t type;
  aid_t aid;
  int retain_privilege;
  lily_error_t error;
  event_handler_item_t* event_handler_head;
  automaton_t* next;
};

struct binding {
  bid_t bid;
  automaton_t* output_automaton;
  char* output_action_begin;
  char* output_action_end;
  ano_t output_action;
  int output_parameter;
  automaton_t* input_automaton;
  char* input_action_begin;
  char* input_action_end;
  ano_t input_action;
  int input_parameter;
  lily_error_t error;
  binding_t* next;
};

struct globbed_binding {
  automaton_t* output_automaton;
  char* output_action_begin;
  char* output_action_end;
  int output_parameter;
  automaton_t* input_automaton;
  char* input_action_begin;
  char* input_action_end;
  int input_parameter;
  lily_error_t error;
  binding_t* binding_head;
  globbed_binding_t* next;
};

void
constellation_init (constellation_t* c)
{
  c->automaton_head = 0;
  c->binding_head = 0;
}

automaton_t*
constellation_add_managed_automaton (constellation_t* c,
				     bd_t bd,
				     int retain_privilege)
{
  automaton_t* a = malloc (sizeof (automaton_t));
  memset (a, 0, sizeof (automaton_t));
  a->type = MANAGED;
  a->aid = -1;
  a->retain_privilege = retain_privilege;

  a->next = c->automaton_head;
  c->automaton_head = a;

  automaton_create (a, bd);

  return a;
}

automaton_t*
constellation_add_unmanaged_automaton (constellation_t* c,
				       aid_t aid)
{
  automaton_t* a = malloc (sizeof (automaton_t));
  memset (a, 0, sizeof (automaton_t));
  a->type = UNMANAGED;
  a->aid = aid;

  a->next = c->automaton_head;
  c->automaton_head = a;

  return a;
}

binding_t*
constellation_add_binding (constellation_t* c,
			   automaton_t* output_automaton,
			   const char* output_action_begin,
			   const char* output_end_end,
			   int output_parameter,
			   automaton_t* input_automaton,
			   const char* input_action_begin,
			   const char* input_end_end,
			   int input_parameter)
{
  /* TODO */
  logs (__func__);
  return 0;
}

static void
binding_bind (binding_t* b)
{
  if (b->output_automaton->aid != -1 && b->input_automaton->aid != -1) {
    if (b->output_action == -1 || b->input_action == -1) {
      /* TODO:  Get the action numbers. */
      logs ("GET THE ACTION NUMBERS");
      exit (-1);
    }

    b->bid = bind (b->output_automaton->aid, b->output_action, b->output_parameter, b->input_automaton->aid, b->input_action, b->input_parameter);
    b->error = lily_error;
    /* TODO:  Probably generate an event. */
  }
}

static binding_t*
create_binding (automaton_t* output_automaton,
		const char* output_action_begin,
		const char* output_action_end,
		ano_t output_action,
		int output_parameter,
		automaton_t* input_automaton,
		const char* input_action_begin,
		const char* input_action_end,
		ano_t input_action,
		int input_parameter)
{
  size_t size;
  binding_t* b = malloc (sizeof (binding_t));
  memset (b, 0, sizeof (binding_t));
  b->bid = -1;
  b->output_automaton = output_automaton;
  size = output_action_end - output_action_begin;
  b->output_action_begin = malloc (size);
  memcpy (b->output_action_begin, output_action_begin, size);
  b->output_action_end = b->output_action_begin + size;
  b->output_action = output_action;
  b->output_parameter = output_parameter;
  b->input_automaton = input_automaton;
  b->input_action = input_action;
  size = input_action_end - input_action_begin;
  b->input_action_begin = malloc (size);
  memcpy (b->input_action_begin, input_action_begin, size);
  b->input_action_end = b->input_action_begin + size;
  b->input_parameter = input_parameter;

  binding_bind (b);

  return b;
}

static void
globbed_binding_bind (globbed_binding_t* b)
{
  if (b->output_automaton->aid != -1 && b->input_automaton->aid != -1) {
    /* Describe the output and input automaton. */
    description_t output_description;
    description_t input_description;
    if (description_init (&output_description, b->output_automaton->aid) != 0) {
      b->error = lily_error;
      return;
    }
    if (description_init (&input_description, b->input_automaton->aid) != 0) {
      description_fini (&output_description);
      b->error = lily_error;
      return;
    }

    size_t output_action_count = description_action_count (&output_description);
    if (output_action_count == -1) {
      description_fini (&output_description);
      description_fini (&input_description);
      b->error = lily_error;
      return;
    }
    
    size_t input_action_count = description_action_count (&input_description);
    if (output_action_count == -1) {
      description_fini (&output_description);
      description_fini (&input_description);
      b->error = lily_error;
      return;
    }
    
    action_desc_t* output_actions = malloc (output_action_count * sizeof (action_desc_t));
    action_desc_t* input_actions = malloc (input_action_count * sizeof (action_desc_t));
        
    if (description_read_all (&output_description, output_actions) != 0) {
      free (output_actions);
      free (input_actions);
      description_fini (&output_description);
      description_fini (&input_description);
      b->error = lily_error;
      return;
    }
    
    if (description_read_all (&input_description, input_actions) != 0) {
      free (output_actions);
      free (input_actions);
      description_fini (&output_description);
      description_fini (&input_description);
      b->error = lily_error;
      return;
    }

    /* Slice and dice the globs. */
    const char* output_glob = pstrchr (b->output_action_begin, b->output_action_end, '*');
    const char* output_prefix_begin = b->output_action_begin;
    const char* output_prefix_end = output_glob;
    const size_t output_prefix_size = output_prefix_end - output_prefix_begin;
    const char* output_suffix_begin = output_glob + 1;
    const char* output_suffix_end = b->output_action_end;
    const size_t output_suffix_size = output_suffix_end - output_suffix_begin;

    const char* input_glob = pstrchr (b->input_action_begin, b->input_action_end, '*');
    const char* input_prefix_begin = b->input_action_begin;
    const char* input_prefix_end = input_glob;
    const size_t input_prefix_size = input_prefix_end - input_prefix_begin;
    const char* input_suffix_begin = input_glob + 1;
    const char* input_suffix_end = b->input_action_end;
    const size_t input_suffix_size = input_suffix_end - input_suffix_begin;

    for (size_t out_idx = 0; out_idx != output_action_count; ++out_idx) {
      if (output_actions[out_idx].type == LILY_ACTION_OUTPUT &&
	  pstrncmp (output_prefix_begin, output_prefix_end, output_actions[out_idx].name, 0, output_prefix_size) == 0 &&
	  pstrncmp (output_suffix_begin, output_suffix_end, output_actions[out_idx].name + output_actions[out_idx].name_size - 1 - output_suffix_size, 0, output_suffix_size) == 0) {

	const char* output_name_begin = output_actions[out_idx].name + output_prefix_size;
	const char* output_name_end = output_name_begin + output_actions[out_idx].name_size - 1 - output_suffix_size;

	for (size_t in_idx = 0; in_idx != input_action_count; ++in_idx) {
	  if (input_actions[in_idx].type == LILY_ACTION_INPUT &&
	      pstrncmp (input_prefix_begin, input_prefix_end, input_actions[in_idx].name, 0, input_prefix_size) == 0 &&
	      pstrncmp (input_suffix_begin, input_suffix_end, input_actions[in_idx].name + input_actions[in_idx].name_size - 1 - input_suffix_size, 0, input_suffix_size) == 0) {
	    
	    const char* input_name_begin = input_actions[in_idx].name + input_prefix_size;
	    const char* input_name_end = input_name_begin + input_actions[in_idx].name_size - 1 - input_suffix_size;
	    
	    if (pstrcmp (output_name_begin, output_name_end, input_name_begin, input_name_end) == 0) {

	      int output_param = 0;
	      int input_param = 0;
	      
	      switch (output_actions[out_idx].parameter_mode) {
	      case NO_PARAMETER:
		output_param = 0;
		break;
	      case PARAMETER:
		output_param = b->output_parameter;
		break;
	      case AUTO_PARAMETER:
		output_param = b->input_automaton->aid;
		break;
	      }
	      
	      switch (input_actions[in_idx].parameter_mode) {
	      case NO_PARAMETER:
		input_param = 0;
		break;
	      case PARAMETER:
		input_param = b->input_parameter;
		break;
	      case AUTO_PARAMETER:
		input_param = b->output_automaton->aid;
		break;
	      }
	      
	      binding_t* binding = create_binding (b->output_automaton, output_actions[out_idx].name, output_actions[out_idx].name + output_actions[out_idx].name_size, output_actions[out_idx].number, output_param, b->input_automaton, input_actions[in_idx].name, input_actions[in_idx].name + input_actions[in_idx].name_size, input_actions[in_idx].number, input_param);
	      binding->next = b->binding_head;
	      b->binding_head = binding;
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
}

static void
automaton_subscribe (automaton_t* a,
		     event_handler_t event_handler,
		     void* arg)
{
  event_handler_item_t* item = malloc (sizeof (event_handler_item_t));
  memset (item, 0, sizeof (event_handler_item_t));
  item->event_handler = event_handler;
  item->arg = arg;

  item->next = a->event_handler_head;
  a->event_handler_head = item;
}

static void
globbed_binding_automaton_event (void* arg)
{
  globbed_binding_t* b = arg;
  globbed_binding_bind (b);
}

globbed_binding_t*
constellation_add_globbed_binding (constellation_t* c,
				   automaton_t* output_automaton,
				   const char* output_action_begin,
				   const char* output_action_end,
				   int output_parameter,
				   automaton_t* input_automaton,
				   const char* input_action_begin,
				   const char* input_action_end,
				   int input_parameter)
{
  globbed_binding_t* b = malloc (sizeof (globbed_binding_t));
  memset (b, 0, sizeof (globbed_binding_t));

  size_t size;

  b->output_automaton = output_automaton;
  size = output_action_end - output_action_begin;
  b->output_action_begin = malloc (size);
  memcpy (b->output_action_begin, output_action_begin, size);
  b->output_action_end = b->output_action_begin + size;
  b->output_parameter = output_parameter;

  b->input_automaton = input_automaton;
  size = input_action_end - input_action_begin;
  b->input_action_begin = malloc (size);
  memcpy (b->input_action_begin, input_action_begin, size);
  b->input_action_end = b->input_action_begin + size;
  b->input_parameter = input_parameter;

  b->next = c->globbed_binding_head;
  c->globbed_binding_head = b;

  automaton_subscribe (output_automaton, globbed_binding_automaton_event, b);
  automaton_subscribe (input_automaton, globbed_binding_automaton_event, b);

  globbed_binding_bind (b);

  return b;
}

void
automaton_create (automaton_t* a,
		  bd_t bd)
{
  if (a->aid == -1 && buffer_size (bd) != -1) {
    a->aid = create (bd, -1, -1, a->retain_privilege);
    a->error = lily_error;
    for (event_handler_item_t* item = a->event_handler_head; item != 0; item = item->next) {
      item->event_handler (item->arg);
    }
  }
}
