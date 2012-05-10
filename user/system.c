/* TODO:  Error checking. */

#include "system.h"
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
  system_t* system;
  automaton_type_t type;
  aid_t aid;
  bd_t text_bd;
  bd_t bda;
  bd_t bdb;
  bool retain_privilege;
  lily_error_t error;
  event_handler_item_t* event_handler_head;
  automaton_t* next;
};

struct binding {
  system_t* system;
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
  bid_t bid;
  lily_error_t error;
  binding_t* next;
  binding_t* bindq_next;
};

/* struct globbed_binding { */
/*   system_t* system; */
/*   automaton_t* output_automaton; */
/*   char* output_action_begin; */
/*   char* output_action_end; */
/*   int output_parameter; */
/*   automaton_t* input_automaton; */
/*   char* input_action_begin; */
/*   char* input_action_end; */
/*   int input_parameter; */
/*   lily_error_t error; */
/*   binding_t* binding_head; */
/*   globbed_binding_t* next; */
/* }; */

void
system_init (system_t* system,
	     buffer_file_t* output_bfa,
	     bd_t bdb,
	     ano_t bind_action)
{
  system->output_bfa = output_bfa;
  system->bdb = bdb;
  system->bind_action = bind_action;
  system->automaton_head = 0;
  system->binding_head = 0;
  system->globbed_binding_head = 0;
  system->this = system_add_unmanaged_automaton (system, getaid ());
  system->bindq_head = 0;
  system->bindq_tail = &system->bindq_head;
}

static void
push_bind (binding_t* bin)
{
  bin->bindq_next = 0;
  *bin->system->bindq_tail = bin;
  bin->system->bindq_tail = &bin->bindq_next;
}

static void
pop_bind (system_t* system)
{
  binding_t* bin = system->bindq_head;
  system->bindq_head = bin->bindq_next;
  if (system->bindq_head == 0) {
    system->bindq_tail = &system->bindq_head;
  }
  bin->bindq_next = 0;
}

static bool
bind_action_precondition (system_t* system)
{
  return system->bindq_head != 0;
}

void
system_bind_action (system_t* system)
{
  if (bind_action_precondition (system)) {
    binding_t* b = system->bindq_head;

    b->bid = bind (b->output_automaton->aid,
		   b->output_action,
		   b->output_parameter,
		   b->input_automaton->aid,
		   b->input_action,
		   b->input_parameter);
    b->error = lily_error;

    /* TODO:  Send bind result to constituents. */
    logs (__func__);

    pop_bind (system);
  }

  finish_internal ();
}

void
system_schedule (system_t* system)
{
  if (bind_action_precondition (system)) {
    schedule (system->bind_action, 0);
  }
}

automaton_t*
system_get_this (system_t* system)
{
  return system->this;
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
automaton_create (automaton_t* a)
{
  if (a->aid == -1 && buffer_size (a->text_bd) != -1) {
    a->aid = create (a->text_bd, a->retain_privilege);
    a->error = lily_error;

    /* TODO:  Initialize. */
    logs (__func__);

    for (event_handler_item_t* e = a->event_handler_head; e != 0; e = e->next) {
      e->event_handler (e->arg);
    }
  }
}

automaton_t*
system_add_managed_automaton (system_t* system,
			      bd_t text_bd,
			      bd_t bda,
			      bd_t bdb,
			      bool retain_privilege)
{
  automaton_t* a = malloc (sizeof (automaton_t));
  memset (a, 0, sizeof (automaton_t));
  a->system = system;
  a->type = MANAGED;
  a->aid = -1;
  a->text_bd = buffer_copy (text_bd);
  a->bda = buffer_copy (bda);
  a->bdb = buffer_copy (bdb);
  a->retain_privilege = retain_privilege;

  a->next = system->automaton_head;
  system->automaton_head = a;

  automaton_create (a);

  return a;
}

automaton_t*
system_add_unmanaged_automaton (system_t* system,
				aid_t aid)
{
  automaton_t* a = malloc (sizeof (automaton_t));
  memset (a, 0, sizeof (automaton_t));
  a->system = system;
  a->type = UNMANAGED;
  a->aid = aid;

  a->next = system->automaton_head;
  system->automaton_head = a;

  return a;
}

static void
binding_bind (binding_t* b)
{
  if (b->output_automaton->aid != -1 && b->input_automaton->aid != -1) {
    /* Describe the output automaton. */
    description_t output_description;
    if (description_init (&output_description, b->output_automaton->aid) != 0) {
      b->error = lily_error;
      return;
    }
    
    /* Look up the actions. */
    action_desc_t output_action;
    if (b->output_action == -1) {
      if (description_read_name (&output_description, &output_action, b->output_action_begin, b->output_action_end) != 0) {
	b->error = lily_error;
	description_fini (&output_description);
	return;
      }
    }
    else {
      if (description_read_number (&output_description, &output_action, b->output_action) != 0) {
	b->error = lily_error;
	description_fini (&output_description);
	return;
      }
    }
    
    b->output_action = output_action.number;
    
    switch (output_action.parameter_mode) {
    case NO_PARAMETER:
      b->output_parameter = 0;
      break;
    case PARAMETER:
      /* Okay. */
      break;
    case AUTO_PARAMETER:
      b->output_parameter = b->input_automaton->aid;
      break;
    }
      
    description_fini (&output_description);

    /* Describe the input automaton. */
    description_t input_description;
    if (description_init (&input_description, b->input_automaton->aid) != 0) {
      b->error = lily_error;
      return;
    }
    
    /* Look up the actions. */
    action_desc_t input_action;
    if (b->input_action == -1) {
      if (description_read_name (&input_description, &input_action, b->input_action_begin, b->input_action_end) != 0) {
	b->error = lily_error;
	description_fini (&input_description);
	return;
      }
    }
    else {
      if (description_read_number (&input_description, &input_action, b->input_action) != 0) {
	b->error = lily_error;
	description_fini (&input_description);
	return;
      }
    }
    
    b->input_action = input_action.number;
    
    switch (input_action.parameter_mode) {
    case NO_PARAMETER:
      b->input_parameter = 0;
      break;
    case PARAMETER:
      /* Okay. */
      break;
    case AUTO_PARAMETER:
      b->input_parameter = b->output_automaton->aid;
      break;
    }
    
    description_fini (&input_description);

    push_bind (b);
  }
}

static binding_t*
create_binding (system_t* system,
		automaton_t* output_automaton,
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
  b->system = system;
  b->output_automaton = output_automaton;
  size = output_action_end - output_action_begin;
  b->output_action_begin = malloc (size);
  memcpy (b->output_action_begin, output_action_begin, size);
  b->output_action_end = b->output_action_begin + size;
  b->output_action = output_action;
  b->output_parameter = output_parameter;
  b->input_automaton = input_automaton;
  size = input_action_end - input_action_begin;
  b->input_action_begin = malloc (size);
  memcpy (b->input_action_begin, input_action_begin, size);
  b->input_action_end = b->input_action_begin + size;
  b->input_action = input_action;
  b->input_parameter = input_parameter;

  b->bid = -1;

  binding_bind (b);

  return b;
}

static void
binding_automaton_event (void* arg)
{
  binding_t* b = arg;
  binding_bind (b);
}

binding_t*
system_add_binding (system_t* system,
		    automaton_t* output_automaton,
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
  if (output_action_begin != 0 && output_action_end == 0) {
    output_action_end = output_action_begin + strlen (output_action_begin);
  }
  if (input_action_begin != 0 && input_action_end == 0) {
    input_action_end = input_action_begin + strlen (input_action_begin);
  }

  binding_t* binding = create_binding (system, output_automaton, output_action_begin, output_action_end, output_action, output_parameter, input_automaton, input_action_begin, input_action_end, input_action, input_parameter);

  binding->next = system->binding_head;
  system->binding_head = binding;

  automaton_subscribe (output_automaton, binding_automaton_event, binding);
  automaton_subscribe (input_automaton, binding_automaton_event, binding);

  return binding;
}

/* static void */
/* globbed_binding_bind (globbed_binding_t* b) */
/* { */
/*   if (b->output_automaton->aid != -1 && b->input_automaton->aid != -1 && (b->owner_automaton == 0 || b->owner_automaton->aid != -1)) { */
/*     /\* Describe the output and input automaton. *\/ */
/*     description_t output_description; */
/*     description_t input_description; */
/*     if (description_init (&output_description, b->output_automaton->aid) != 0) { */
/*       b->error = lily_error; */
/*       return; */
/*     } */
/*     if (description_init (&input_description, b->input_automaton->aid) != 0) { */
/*       description_fini (&output_description); */
/*       b->error = lily_error; */
/*       return; */
/*     } */

/*     size_t output_action_count = description_action_count (&output_description); */
/*     if (output_action_count == -1) { */
/*       description_fini (&output_description); */
/*       description_fini (&input_description); */
/*       b->error = lily_error; */
/*       return; */
/*     } */
    
/*     size_t input_action_count = description_action_count (&input_description); */
/*     if (output_action_count == -1) { */
/*       description_fini (&output_description); */
/*       description_fini (&input_description); */
/*       b->error = lily_error; */
/*       return; */
/*     } */
    
/*     action_desc_t* output_actions = malloc (output_action_count * sizeof (action_desc_t)); */
/*     action_desc_t* input_actions = malloc (input_action_count * sizeof (action_desc_t)); */
        
/*     if (description_read_all (&output_description, output_actions) != 0) { */
/*       free (output_actions); */
/*       free (input_actions); */
/*       description_fini (&output_description); */
/*       description_fini (&input_description); */
/*       b->error = lily_error; */
/*       return; */
/*     } */
    
/*     if (description_read_all (&input_description, input_actions) != 0) { */
/*       free (output_actions); */
/*       free (input_actions); */
/*       description_fini (&output_description); */
/*       description_fini (&input_description); */
/*       b->error = lily_error; */
/*       return; */
/*     } */

/*     /\* Slice and dice the globs. *\/ */
/*     const char* output_glob = pstrchr (b->output_action_begin, b->output_action_end, '*'); */
/*     const char* output_prefix_begin = b->output_action_begin; */
/*     const char* output_prefix_end = output_glob; */
/*     const size_t output_prefix_size = output_prefix_end - output_prefix_begin; */
/*     const char* output_suffix_begin = output_glob + 1; */
/*     const char* output_suffix_end = b->output_action_end; */
/*     const size_t output_suffix_size = output_suffix_end - output_suffix_begin; */

/*     const char* input_glob = pstrchr (b->input_action_begin, b->input_action_end, '*'); */
/*     const char* input_prefix_begin = b->input_action_begin; */
/*     const char* input_prefix_end = input_glob; */
/*     const size_t input_prefix_size = input_prefix_end - input_prefix_begin; */
/*     const char* input_suffix_begin = input_glob + 1; */
/*     const char* input_suffix_end = b->input_action_end; */
/*     const size_t input_suffix_size = input_suffix_end - input_suffix_begin; */

/*     for (size_t out_idx = 0; out_idx != output_action_count; ++out_idx) { */
/*       if (output_actions[out_idx].type == LILY_ACTION_OUTPUT && */
/* 	  pstrncmp (output_prefix_begin, output_prefix_end, output_actions[out_idx].name_begin, output_actions[out_idx].name_end, output_prefix_size) == 0 && */
/* 	  pstrncmp (output_suffix_begin, output_suffix_end, output_actions[out_idx].name_end - output_suffix_size, output_actions[out_idx].name_end, output_suffix_size) == 0) { */

/* 	const char* output_name_begin = output_actions[out_idx].name_begin + output_prefix_size; */
/* 	const char* output_name_end = output_actions[out_idx].name_end - output_suffix_size; */

/* 	for (size_t in_idx = 0; in_idx != input_action_count; ++in_idx) { */
/* 	  if (input_actions[in_idx].type == LILY_ACTION_INPUT && */
/* 	      pstrncmp (input_prefix_begin, input_prefix_end, input_actions[in_idx].name_begin, input_actions[in_idx].name_end, input_prefix_size) == 0 && */
/* 	      pstrncmp (input_suffix_begin, input_suffix_end, input_actions[in_idx].name_end - input_suffix_size, input_actions[in_idx].name_end, input_suffix_size) == 0) { */
	    
/* 	    const char* input_name_begin = input_actions[in_idx].name_begin + input_prefix_size; */
/* 	    const char* input_name_end = input_actions[in_idx].name_end - input_suffix_size; */
	    
/* 	    if (pstrcmp (output_name_begin, output_name_end, input_name_begin, input_name_end) == 0) { */

/* 	      int output_param = 0; */
/* 	      int input_param = 0; */
	      
/* 	      switch (output_actions[out_idx].parameter_mode) { */
/* 	      case NO_PARAMETER: */
/* 		output_param = 0; */
/* 		break; */
/* 	      case PARAMETER: */
/* 		output_param = b->output_parameter; */
/* 		break; */
/* 	      case AUTO_PARAMETER: */
/* 		output_param = b->input_automaton->aid; */
/* 		break; */
/* 	      } */
	      
/* 	      switch (input_actions[in_idx].parameter_mode) { */
/* 	      case NO_PARAMETER: */
/* 		input_param = 0; */
/* 		break; */
/* 	      case PARAMETER: */
/* 		input_param = b->input_parameter; */
/* 		break; */
/* 	      case AUTO_PARAMETER: */
/* 		input_param = b->output_automaton->aid; */
/* 		break; */
/* 	      } */
	      
/* 	      binding_t* binding = create_binding (b->system, b->output_automaton, output_actions[out_idx].name_begin, output_actions[out_idx].name_end, output_actions[out_idx].number, output_param, b->input_automaton, input_actions[in_idx].name_begin, input_actions[in_idx].name_end, input_actions[in_idx].number, input_param, b->owner_automaton); */
/* 	      binding->next = b->binding_head; */
/* 	      b->binding_head = binding; */
/* 	    } */
/* 	  } */
/* 	} */
/*       } */
/*     } */

/*     free (output_actions); */
/*     free (input_actions); */
      
/*     description_fini (&output_description); */
/*     description_fini (&input_description); */
/*   } */
/* } */

/* static void */
/* globbed_binding_automaton_event (void* arg) */
/* { */
/*   globbed_binding_t* b = arg; */
/*   globbed_binding_bind (b); */
/* } */

/* globbed_binding_t* */
/* system_add_globbed_binding (system_t* system, */
/* 			    automaton_t* output_automaton, */
/* 			    const char* output_action_begin, */
/* 			    const char* output_action_end, */
/* 			    int output_parameter, */
/* 			    automaton_t* input_automaton, */
/* 			    const char* input_action_begin, */
/* 			    const char* input_action_end, */
/* 			    int input_parameter, */
/* 			    automaton_t* owner_automaton) */
/* { */
/*   if (output_action_end == 0) { */
/*     output_action_end = output_action_begin + strlen (output_action_begin); */
/*   } */
/*   if (input_action_end == 0) { */
/*     input_action_end = input_action_begin + strlen (input_action_begin); */
/*   } */

/*   globbed_binding_t* b = malloc (sizeof (globbed_binding_t)); */
/*   memset (b, 0, sizeof (globbed_binding_t)); */

/*   size_t size; */

/*   b->system = system; */
/*   b->output_automaton = output_automaton; */
/*   size = output_action_end - output_action_begin; */
/*   b->output_action_begin = malloc (size); */
/*   memcpy (b->output_action_begin, output_action_begin, size); */
/*   b->output_action_end = b->output_action_begin + size; */
/*   b->output_parameter = output_parameter; */

/*   b->input_automaton = input_automaton; */
/*   size = input_action_end - input_action_begin; */
/*   b->input_action_begin = malloc (size); */
/*   memcpy (b->input_action_begin, input_action_begin, size); */
/*   b->input_action_end = b->input_action_begin + size; */
/*   b->input_parameter = input_parameter; */
  
/*   b->owner_automaton = owner_automaton; */

/*   b->next = system->globbed_binding_head; */
/*   system->globbed_binding_head = b; */

/*   automaton_subscribe (output_automaton, globbed_binding_automaton_event, b); */
/*   automaton_subscribe (input_automaton, globbed_binding_automaton_event, b); */
/*   if (owner_automaton != 0) { */
/*     automaton_subscribe (owner_automaton, globbed_binding_automaton_event, b); */
/*   } */

/*   globbed_binding_bind (b); */

/*   return b; */
/* } */

void
automaton_set_text (automaton_t* a,
		    bd_t text_bd)
{
  buffer_destroy (a->text_bd);
  a->text_bd = buffer_copy (text_bd);
  automaton_create (a);
}

bool
binding_bound (const binding_t* binding)
{
  return binding->bid != -1;
}
