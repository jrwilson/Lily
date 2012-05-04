/* TODO:  Error checking. */

#include "system.h"
#include <dymem.h>
#include <string.h>
#include <description.h>
#include "system_msg.h"

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
  int retain_privilege;
  automaton_t* owner_automaton;
  lily_error_t error;
  sa_create_outcome_t outcome;
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
  automaton_t* owner_automaton;
  lily_error_t error;
  sa_bind_outcome_t outcome;
  binding_t* next;
};

/* struct globbed_binding { */
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
	     ano_t create_request,
	     ano_t bind_request)
{
  system->output_bfa = output_bfa;
  system->bdb = bdb;
  system->create_request = create_request;
  system->bind_request = bind_request;
  system->automaton_head = 0;
  system->binding_head = 0;
  system->globbed_binding_head = 0;
  system->this = system_add_unmanaged_automaton (system, getaid ());
  system->bind_request_head = 0;
  system->bind_request_tail = &system->bind_request_head;
  system->bind_response_head = 0;
  system->bind_response_tail = &system->bind_response_head;
  system->create_request_head = 0;
  system->create_request_tail = &system->create_request_head;
  system->create_response_head = 0;
  system->create_response_tail = &system->create_response_head;
}

struct create_request {
  automaton_t* automaton;
  sa_create_request_t* request;
  create_request_t* next;
};

static void
push_create (automaton_t* automaton)
{
  create_request_t* req = malloc (sizeof (create_request_t));
  memset (req, 0, sizeof (create_request_t));
  req->automaton = automaton;
  req->request = sa_create_request_create (automaton->text_bd, automaton->bda, automaton->bdb, automaton->retain_privilege, (automaton->owner_automaton != 0) ? automaton->owner_automaton->aid : -1);
  buffer_destroy (automaton->text_bd);
  buffer_destroy (automaton->bda);
  buffer_destroy (automaton->bdb);
  *automaton->system->create_request_tail = req;
  automaton->system->create_request_tail = &req->next;
}

static void
shift_create (system_t* system)
{
  create_request_t* req = system->create_request_head;
  system->create_request_head = req->next;
  if (system->create_request_head == 0) {
    system->create_request_tail = &system->create_request_head;
  }
  req->next = 0;

  *system->create_response_tail = req;
  system->create_response_tail = &req->next;
}

static void
pop_create (system_t* system)
{
  create_request_t* req = system->create_response_head;
  system->create_response_head = req->next;
  if (system->create_response_head == 0) {
    system->create_response_tail = &system->create_response_head;
  }

  sa_create_request_destroy (req->request);
  free (req);
}

static bool
create_request_precondition (system_t* system)
{
  return system->create_request_head != 0;
}

void
system_create_request (system_t* system)
{
  if (create_request_precondition (system)) {
    /* Shred the output buffer. */
    buffer_file_shred (system->output_bfa);

    /* Write the request. */
    sa_create_request_write (system->output_bfa, system->bdb, system->create_request_head->request);

    /* Shift the request. */
    shift_create (system);

    finish_output (true, system->output_bfa->bd, system->bdb);
  }
  finish_output (false, -1, -1);
}

void
system_create_response (system_t* system,
			bd_t bda,
			bd_t bdb)
{
  if (system->create_response_head != 0) {
    automaton_t* automaton = system->create_response_head->automaton;

    buffer_file_t bf;
    if (buffer_file_initr (&bf, bda) != 0) {
      finish_input (bda, bdb);
    }

    const sa_create_response_t* res = buffer_file_readp (&bf, sizeof (sa_create_response_t));
    if (res == 0) {
      finish_input (bda, bdb);
    }

    automaton->aid = res->aid;
    automaton->outcome = res->outcome;

    /* TODO:  Trigger an event, callbacks, etc.. */
    
    for (event_handler_item_t* item = automaton->event_handler_head; item != 0; item = item->next) {
      item->event_handler (item->arg);
    }

    pop_create (system);
  }
  finish_input (bda, bdb);
}

struct bind_request {
  binding_t* binding;
  sa_bind_request_t request;
  bind_request_t* next;
};

static void
push_bind (binding_t* bin,
	   const sa_binding_t* binding)
{
  bind_request_t* req = malloc (sizeof (bind_request_t));
  memset (req, 0, sizeof (bind_request_t));
  req->binding = bin;
  sa_bind_request_init (&req->request, binding);
  *bin->system->bind_request_tail = req;
  bin->system->bind_request_tail = &req->next;
}

static void
shift_bind (system_t* system)
{
  bind_request_t* req = system->bind_request_head;
  system->bind_request_head = req->next;
  if (system->bind_request_head == 0) {
    system->bind_request_tail = &system->bind_request_head;
  }
  req->next = 0;

  *system->bind_response_tail = req;
  system->bind_response_tail = &req->next;
}

static void
pop_bind (system_t* system)
{
  bind_request_t* req = system->bind_response_head;
  system->bind_response_head = req->next;
  if (system->bind_response_head == 0) {
    system->bind_response_tail = &system->bind_response_head;
  }
  free (req);
}

static bool
bind_request_precondition (system_t* system)
{
  return system->bind_request_head != 0;
}

void
system_bind_request (system_t* system)
{
  if (bind_request_precondition (system)) {
    /* Shred the output buffer. */
    buffer_file_shred (system->output_bfa);

    /* Write the request. */
    buffer_file_write (system->output_bfa, &system->bind_request_head->request, sizeof (sa_bind_request_t));

    /* Shift the request. */
    shift_bind (system);

    finish_output (true, system->output_bfa->bd, -1);
  }
  finish_output (false, -1, -1);
}

void
system_bind_response (system_t* system,
		      bd_t bda,
		      bd_t bdb)
{
  if (system->bind_response_head != 0) {
    buffer_file_t bf;
    if (buffer_file_initr (&bf, bda) != 0) {
      finish_input (bda, bdb);
    }

    const sa_bind_response_t* res = buffer_file_readp (&bf, sizeof (sa_bind_response_t));
    if (res == 0) {
      finish_input (bda, bdb);
    }

    system->bind_response_head->binding->outcome = res->outcome;

    /* TODO:  Trigger an event, callbacks, etc.. */

    pop_bind (system);
  }
  finish_input (bda, bdb);
}

void
system_schedule (system_t* system)
{
  if (create_request_precondition (system)) {
    schedule (system->create_request, 0);
  }
  if (bind_request_precondition (system)) {
    schedule (system->bind_request, 0);
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
  if (a->aid == -1 && buffer_size (a->text_bd) != -1 && (a->owner_automaton == 0 || a->owner_automaton->aid != -1)) {
    push_create (a);
  }
}

static void
automaton_owner_event (void* arg)
{
  automaton_t* a = arg;
  automaton_create (a);
}

automaton_t*
system_add_managed_automaton (system_t* system,
			      bd_t text_bd,
			      bd_t bda,
			      bd_t bdb,
			      int retain_privilege,
			      automaton_t* owner)
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
  a->owner_automaton = owner;

  if (owner != 0) {
    automaton_subscribe (owner, automaton_owner_event, a);
  }

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
  if (b->output_automaton->aid != -1 && b->input_automaton->aid != -1 && (b->owner_automaton == 0 || b->owner_automaton->aid != -1)) {
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

    sa_binding_t sab;
    sa_binding_init (&sab, b->output_automaton->aid, b->output_action, b->output_parameter, b->input_automaton->aid, b->input_action, b->input_parameter, b->owner_automaton->aid);

    push_bind (b, &sab);
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
		int input_parameter,
		automaton_t* owner_automaton)
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
  b->owner_automaton = owner_automaton;

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
		    int input_parameter,
		    automaton_t* owner_automaton)
{
  if (output_action_begin != 0 && output_action_end == 0) {
    output_action_end = output_action_begin + strlen (output_action_begin);
  }
  if (input_action_begin != 0 && input_action_end == 0) {
    input_action_end = input_action_begin + strlen (input_action_begin);
  }

  binding_t* binding = create_binding (system, output_automaton, output_action_begin, output_action_end, output_action, output_parameter, input_automaton, input_action_begin, input_action_end, input_action, input_parameter, owner_automaton);

  binding->next = system->binding_head;
  system->binding_head = binding;

  automaton_subscribe (output_automaton, binding_automaton_event, binding);
  automaton_subscribe (input_automaton, binding_automaton_event, binding);

  return binding;
}

/* /\* static void *\/ */
/* /\* globbed_binding_bind (globbed_binding_t* b) *\/ */
/* /\* { *\/ */
/* /\*   if (b->output_automaton->aid != -1 && b->input_automaton->aid != -1) { *\/ */
/* /\*     /\\* Describe the output and input automaton. *\\/ *\/ */
/* /\*     description_t output_description; *\/ */
/* /\*     description_t input_description; *\/ */
/* /\*     if (description_init (&output_description, b->output_automaton->aid) != 0) { *\/ */
/* /\*       b->error = lily_error; *\/ */
/* /\*       return; *\/ */
/* /\*     } *\/ */
/* /\*     if (description_init (&input_description, b->input_automaton->aid) != 0) { *\/ */
/* /\*       description_fini (&output_description); *\/ */
/* /\*       b->error = lily_error; *\/ */
/* /\*       return; *\/ */
/* /\*     } *\/ */

/* /\*     size_t output_action_count = description_action_count (&output_description); *\/ */
/* /\*     if (output_action_count == -1) { *\/ */
/* /\*       description_fini (&output_description); *\/ */
/* /\*       description_fini (&input_description); *\/ */
/* /\*       b->error = lily_error; *\/ */
/* /\*       return; *\/ */
/* /\*     } *\/ */
    
/* /\*     size_t input_action_count = description_action_count (&input_description); *\/ */
/* /\*     if (output_action_count == -1) { *\/ */
/* /\*       description_fini (&output_description); *\/ */
/* /\*       description_fini (&input_description); *\/ */
/* /\*       b->error = lily_error; *\/ */
/* /\*       return; *\/ */
/* /\*     } *\/ */
    
/* /\*     action_desc_t* output_actions = malloc (output_action_count * sizeof (action_desc_t)); *\/ */
/* /\*     action_desc_t* input_actions = malloc (input_action_count * sizeof (action_desc_t)); *\/ */
        
/* /\*     if (description_read_all (&output_description, output_actions) != 0) { *\/ */
/* /\*       free (output_actions); *\/ */
/* /\*       free (input_actions); *\/ */
/* /\*       description_fini (&output_description); *\/ */
/* /\*       description_fini (&input_description); *\/ */
/* /\*       b->error = lily_error; *\/ */
/* /\*       return; *\/ */
/* /\*     } *\/ */
    
/* /\*     if (description_read_all (&input_description, input_actions) != 0) { *\/ */
/* /\*       free (output_actions); *\/ */
/* /\*       free (input_actions); *\/ */
/* /\*       description_fini (&output_description); *\/ */
/* /\*       description_fini (&input_description); *\/ */
/* /\*       b->error = lily_error; *\/ */
/* /\*       return; *\/ */
/* /\*     } *\/ */

/* /\*     /\\* Slice and dice the globs. *\\/ *\/ */
/* /\*     const char* output_glob = pstrchr (b->output_action_begin, b->output_action_end, '*'); *\/ */
/* /\*     const char* output_prefix_begin = b->output_action_begin; *\/ */
/* /\*     const char* output_prefix_end = output_glob; *\/ */
/* /\*     const size_t output_prefix_size = output_prefix_end - output_prefix_begin; *\/ */
/* /\*     const char* output_suffix_begin = output_glob + 1; *\/ */
/* /\*     const char* output_suffix_end = b->output_action_end; *\/ */
/* /\*     const size_t output_suffix_size = output_suffix_end - output_suffix_begin; *\/ */

/* /\*     const char* input_glob = pstrchr (b->input_action_begin, b->input_action_end, '*'); *\/ */
/* /\*     const char* input_prefix_begin = b->input_action_begin; *\/ */
/* /\*     const char* input_prefix_end = input_glob; *\/ */
/* /\*     const size_t input_prefix_size = input_prefix_end - input_prefix_begin; *\/ */
/* /\*     const char* input_suffix_begin = input_glob + 1; *\/ */
/* /\*     const char* input_suffix_end = b->input_action_end; *\/ */
/* /\*     const size_t input_suffix_size = input_suffix_end - input_suffix_begin; *\/ */

/* /\*     for (size_t out_idx = 0; out_idx != output_action_count; ++out_idx) { *\/ */
/* /\*       if (output_actions[out_idx].type == LILY_ACTION_OUTPUT && *\/ */
/* /\* 	  pstrncmp (output_prefix_begin, output_prefix_end, output_actions[out_idx].name_begin, output_actions[out_idx].name_end, output_prefix_size) == 0 && *\/ */
/* /\* 	  pstrncmp (output_suffix_begin, output_suffix_end, output_actions[out_idx].name_end - output_suffix_size, output_actions[out_idx].name_end, output_suffix_size) == 0) { *\/ */

/* /\* 	const char* output_name_begin = output_actions[out_idx].name_begin + output_prefix_size; *\/ */
/* /\* 	const char* output_name_end = output_actions[out_idx].name_end - output_suffix_size; *\/ */

/* /\* 	for (size_t in_idx = 0; in_idx != input_action_count; ++in_idx) { *\/ */
/* /\* 	  if (input_actions[in_idx].type == LILY_ACTION_INPUT && *\/ */
/* /\* 	      pstrncmp (input_prefix_begin, input_prefix_end, input_actions[in_idx].name_begin, input_actions[in_idx].name_end, input_prefix_size) == 0 && *\/ */
/* /\* 	      pstrncmp (input_suffix_begin, input_suffix_end, input_actions[in_idx].name_end - input_suffix_size, input_actions[in_idx].name_end, input_suffix_size) == 0) { *\/ */
	    
/* /\* 	    const char* input_name_begin = input_actions[in_idx].name_begin + input_prefix_size; *\/ */
/* /\* 	    const char* input_name_end = input_actions[in_idx].name_end - input_suffix_size; *\/ */
	    
/* /\* 	    if (pstrcmp (output_name_begin, output_name_end, input_name_begin, input_name_end) == 0) { *\/ */

/* /\* 	      int output_param = 0; *\/ */
/* /\* 	      int input_param = 0; *\/ */
	      
/* /\* 	      switch (output_actions[out_idx].parameter_mode) { *\/ */
/* /\* 	      case NO_PARAMETER: *\/ */
/* /\* 		output_param = 0; *\/ */
/* /\* 		break; *\/ */
/* /\* 	      case PARAMETER: *\/ */
/* /\* 		output_param = b->output_parameter; *\/ */
/* /\* 		break; *\/ */
/* /\* 	      case AUTO_PARAMETER: *\/ */
/* /\* 		output_param = b->input_automaton->aid; *\/ */
/* /\* 		break; *\/ */
/* /\* 	      } *\/ */
	      
/* /\* 	      switch (input_actions[in_idx].parameter_mode) { *\/ */
/* /\* 	      case NO_PARAMETER: *\/ */
/* /\* 		input_param = 0; *\/ */
/* /\* 		break; *\/ */
/* /\* 	      case PARAMETER: *\/ */
/* /\* 		input_param = b->input_parameter; *\/ */
/* /\* 		break; *\/ */
/* /\* 	      case AUTO_PARAMETER: *\/ */
/* /\* 		input_param = b->output_automaton->aid; *\/ */
/* /\* 		break; *\/ */
/* /\* 	      } *\/ */
	      
/* /\* 	      binding_t* binding = create_binding (b->output_automaton, output_actions[out_idx].name_begin, output_actions[out_idx].name_end, output_actions[out_idx].number, output_param, b->input_automaton, input_actions[in_idx].name_begin, input_actions[in_idx].name_end, input_actions[in_idx].number, input_param); *\/ */
/* /\* 	      binding->next = b->binding_head; *\/ */
/* /\* 	      b->binding_head = binding; *\/ */
/* /\* 	    } *\/ */
/* /\* 	  } *\/ */
/* /\* 	} *\/ */
/* /\*       } *\/ */
/* /\*     } *\/ */

/* /\*     free (output_actions); *\/ */
/* /\*     free (input_actions); *\/ */
      
/* /\*     description_fini (&output_description); *\/ */
/* /\*     description_fini (&input_description); *\/ */
/* /\*   } *\/ */
/* /\* } *\/ */


/* /\* static void *\/ */
/* /\* globbed_binding_automaton_event (void* arg) *\/ */
/* /\* { *\/ */
/* /\*   globbed_binding_t* b = arg; *\/ */
/* /\*   globbed_binding_bind (b); *\/ */
/* /\* } *\/ */

/* /\* globbed_binding_t* *\/ */
/* /\* system_add_globbed_binding (system_t* c, *\/ */
/* /\* 				   automaton_t* output_automaton, *\/ */
/* /\* 				   const char* output_action_begin, *\/ */
/* /\* 				   const char* output_action_end, *\/ */
/* /\* 				   int output_parameter, *\/ */
/* /\* 				   automaton_t* input_automaton, *\/ */
/* /\* 				   const char* input_action_begin, *\/ */
/* /\* 				   const char* input_action_end, *\/ */
/* /\* 				   int input_parameter) *\/ */
/* /\* { *\/ */
/* /\*   if (output_action_end == 0) { *\/ */
/* /\*     output_action_end = output_action_begin + strlen (output_action_begin); *\/ */
/* /\*   } *\/ */
/* /\*   if (input_action_end == 0) { *\/ */
/* /\*     input_action_end = input_action_begin + strlen (input_action_begin); *\/ */
/* /\*   } *\/ */

/* /\*   globbed_binding_t* b = malloc (sizeof (globbed_binding_t)); *\/ */
/* /\*   memset (b, 0, sizeof (globbed_binding_t)); *\/ */

/* /\*   size_t size; *\/ */

/* /\*   b->output_automaton = output_automaton; *\/ */
/* /\*   size = output_action_end - output_action_begin; *\/ */
/* /\*   b->output_action_begin = malloc (size); *\/ */
/* /\*   memcpy (b->output_action_begin, output_action_begin, size); *\/ */
/* /\*   b->output_action_end = b->output_action_begin + size; *\/ */
/* /\*   b->output_parameter = output_parameter; *\/ */

/* /\*   b->input_automaton = input_automaton; *\/ */
/* /\*   size = input_action_end - input_action_begin; *\/ */
/* /\*   b->input_action_begin = malloc (size); *\/ */
/* /\*   memcpy (b->input_action_begin, input_action_begin, size); *\/ */
/* /\*   b->input_action_end = b->input_action_begin + size; *\/ */
/* /\*   b->input_parameter = input_parameter; *\/ */

/* /\*   b->next = c->globbed_binding_head; *\/ */
/* /\*   c->globbed_binding_head = b; *\/ */

/* /\*   automaton_subscribe (output_automaton, globbed_binding_automaton_event, b); *\/ */
/* /\*   automaton_subscribe (input_automaton, globbed_binding_automaton_event, b); *\/ */

/* /\*   globbed_binding_bind (b); *\/ */

/* /\*   return b; *\/ */
/* /\* } *\/ */

void
automaton_set_text (automaton_t* a,
		    bd_t text_bd)
{
  buffer_destroy (a->text_bd);
  a->text_bd = buffer_copy (text_bd);
  automaton_create (a);
}
