#include "system.h"
#include <dymem.h>
#include <string.h>
#include "buffer_file.h"

struct action {
  int type;
  int parameter_mode;
  ano_t number;
  char* name_begin;
  char* name_end;
  char* description_begin;
  char* description_end;
  action_t* next;
};

struct automaton {
  aid_t aid;
  lily_error_t error;
  action_t* action_head;
  automaton_t* next;
};

struct binding {
  bid_t bid;
  lily_error_t error;
  const automaton_t* output_automaton;
  char* output_begin;
  char* output_end;
  ano_t output_action;
  int output_parameter;
  const automaton_t* input_automaton;
  char* input_begin;
  char* input_end;
  ano_t input_action;
  int input_parameter;
  binding_t* next;
};

struct create_item {
  automaton_t* automaton;
  bd_t text_bd;
  bool retain_privilege;
  bd_t bda;
  bd_t bdb;
  create_callback_t callback;
  void* arg;
  create_item_t* next;
};

struct bind_item {
  binding_t* binding;
  bind_item_t* next;
};

struct init_item {
  automaton_t* automaton;
  bd_t bda;
  bd_t bdb;
  init_item_t* next;
};

/* /\* struct globbed_binding { *\/ */
/* /\*   system_t* system; *\/ */
/* /\*   automaton_t* output_automaton; *\/ */
/* /\*   char* output_action_begin; *\/ */
/* /\*   char* output_action_end; *\/ */
/* /\*   int output_parameter; *\/ */
/* /\*   automaton_t* input_automaton; *\/ */
/* /\*   char* input_action_begin; *\/ */
/* /\*   char* input_action_end; *\/ */
/* /\*   int input_parameter; *\/ */
/* /\*   lily_error_t error; *\/ */
/* /\*   binding_t* binding_head; *\/ */
/* /\*   globbed_binding_t* next; *\/ */
/* /\* }; *\/ */

static automaton_t*
automaton_create (system_t* system)
{
  automaton_t* a = malloc (sizeof (automaton_t));
  memset (a, 0, sizeof (automaton_t));
  
  a->aid = -1;
  a->next = system->automaton_head;
  system->automaton_head = a;

  return a;
}

aid_t
automaton_aid (const automaton_t* a)
{
  return a->aid;
}

lily_error_t
automaton_error (const automaton_t* a)
{
  return a->error;
}

static const action_t*
automaton_find_action (const automaton_t* a,
		       const char* name_begin,
		       const char* name_end)
{
  if (name_end == 0) {
    name_end = name_begin + strlen (name_begin);
  }

  for (const action_t* ac = a->action_head; ac != 0; ac = ac->next) {
    if (pstrcmp (ac->name_begin, ac->name_end, name_begin, name_end) == 0) {
      return ac;
    }
  }

  return 0;
}

static void
automaton_set_aid (automaton_t* a,
		   aid_t aid,
		   lily_error_t error)
{
  a->aid = aid;
  a->error = error;

  if (aid != -1) {
    bd_t bd = describe (aid);
    if (bd == -1) {
      /* Something bad happened.  Do not continue. */
      return;
    }

    buffer_file_t bf;
    if (buffer_file_initr (&bf, bd) != 0) {
      buffer_destroy (bd);
      return;
    }

    size_t count;
    if (buffer_file_read (&bf, &count, sizeof (size_t)) != 0) {
      return;
    }

    for (size_t idx = 0; idx != count; ++idx) {
      const action_descriptor_t* ad = buffer_file_readp (&bf, sizeof (action_descriptor_t));
      if (ad == 0) {
	return;
      }
      
      const char* name = buffer_file_readp (&bf, ad->name_size);
      if (name == 0) {
	return;
      }
      
      const char* description = buffer_file_readp (&bf, ad->description_size);
      if (description == 0) {
	return;
      }
      
      action_t* action = malloc (sizeof (action_t));
      memset (action, 0, sizeof (action_t));

      action->type = ad->type;
      action->parameter_mode = ad->parameter_mode;
      action->number = ad->number;
      action->name_begin = malloc (ad->name_size);
      memcpy (action->name_begin, name, ad->name_size);
      action->name_end = action->name_begin + ad->name_size - 1; // Adjust for null terminator.
      action->description_begin = malloc (ad->description_size);
      memcpy (action->description_begin, description, ad->description_size);
      action->description_end = action->description_begin + ad->description_size - 1; // Adjust for null terminator.
      action->next = a->action_head;
      a->action_head = action;
    }

    buffer_destroy (bd);
  }
}

static int
binding_resolve (binding_t* b)
{
  if (b->output_automaton->aid == -1) {
    return -1;
  }
  const action_t* output_action = automaton_find_action (b->output_automaton, b->output_begin, b->output_end);
  if (output_action == 0) {
    return -1;
  }
  if (output_action->type != LILY_ACTION_OUTPUT) {
    return -1;
  }

  if (b->input_automaton->aid == -1) {
    return -1;
  }
  const action_t* input_action = automaton_find_action (b->input_automaton, b->input_begin, b->input_end);
  if (output_action == 0) {
    return -1;
  }
  if (input_action->type != LILY_ACTION_INPUT) {
    return -1;
  }

  switch (output_action->parameter_mode) {
  case NO_PARAMETER:
    b->output_parameter = 0;
    break;
  case PARAMETER:
    break;
  case AUTO_PARAMETER:
    b->output_parameter = b->input_automaton->aid;
    break;
  }

  switch (input_action->parameter_mode) {
  case NO_PARAMETER:
    b->input_parameter = 0;
    break;
  case PARAMETER:
    break;
  case AUTO_PARAMETER:
    b->input_parameter = b->output_automaton->aid;
    break;
  }

  b->output_action = output_action->number;
  b->input_action = input_action->number;

  return 0;
}

static int
automaton_interface_check (const automaton_t* a,
			   system_t* system)
{
  const action_t* system_action = automaton_find_action (a, SA_SYSTEM_ACTION_NAME, 0);
  const action_t* init_out = automaton_find_action (a, SA_INIT_OUT_NAME, 0);
  const action_t* init_in = automaton_find_action (a, SA_INIT_IN_NAME, 0);
  const action_t* status_request_out = automaton_find_action (a, SA_STATUS_REQUEST_OUT_NAME, 0);
  const action_t* status_request_in = automaton_find_action (a, SA_STATUS_REQUEST_IN_NAME, 0);
  const action_t* status_response_out = automaton_find_action (a, SA_STATUS_RESPONSE_OUT_NAME, 0);
  const action_t* status_response_in = automaton_find_action (a, SA_STATUS_RESPONSE_IN_NAME, 0);
  const action_t* binding_update_out = automaton_find_action (a, SA_BINDING_UPDATE_OUT_NAME, 0);
  const action_t* binding_update_in = automaton_find_action (a, SA_BINDING_UPDATE_IN_NAME, 0);

  if (system_action == 0 ||
      system_action->type != LILY_ACTION_SYSTEM) {
    return -1;
  }

  if (init_out == 0 ||
      init_out->type != LILY_ACTION_OUTPUT ||
      init_out->parameter_mode != AUTO_PARAMETER) {
    return -1;
  }

  if (init_in == 0 ||
      init_in->type != LILY_ACTION_INPUT) {
    return -1;
  }

  if (status_request_out == 0 ||
      status_request_out->type != LILY_ACTION_OUTPUT ||
      status_request_out->parameter_mode != AUTO_PARAMETER) {
    return -1;
  }

  if (status_request_in == 0 ||
      status_request_in->type != LILY_ACTION_INPUT ||
      status_request_in->parameter_mode != AUTO_PARAMETER) {
    return -1;
  }

  if (status_response_out == 0 ||
      status_response_out->type != LILY_ACTION_OUTPUT ||
      status_response_out->parameter_mode != AUTO_PARAMETER) {
    return -1;
  }

  if (status_response_in == 0 ||
      status_response_in->type != LILY_ACTION_INPUT ||
      status_response_in->parameter_mode != AUTO_PARAMETER) {
    return -1;
  }

  if (binding_update_out == 0 ||
      binding_update_out->type != LILY_ACTION_OUTPUT ||
      binding_update_out->parameter_mode != AUTO_PARAMETER) {
    return -1;
  }

  if (binding_update_in == 0 ||
      binding_update_in->type != LILY_ACTION_INPUT ||
      binding_update_in->parameter_mode != AUTO_PARAMETER) {
    return -1;
  }

  if (system != 0) {
    system->system_action = system_action->number;
    system->init_out = init_out->number;
  }

  return 0;
}

int
system_init (system_t* system)
{
  system->automaton_head = 0;
  system->binding_head = 0;
  system->createq_head = 0;
  system->createq_tail = &system->createq_head;
  system->bindq_head = 0;
  system->bindq_tail = &system->bindq_head;
  system->initq_head = 0;
  system->initq_tail = &system->initq_head;
  system->this = automaton_create (system);
  automaton_set_aid (system->this, getaid (), LILY_ERROR_SUCCESS);

  if (automaton_interface_check (system->this, system) != 0) {
    return -1;
  }

  system->init_bda = buffer_create (0);
  system->init_bdb = buffer_create (0);
  /* system->globbed_binding_head = 0; */

  return 0;
}

static void
push_create (system_t* system,
	     automaton_t* a,
	     bd_t text_bd,
	     bool retain_privilege,
	     bd_t bda,
	     bd_t bdb,
	     create_callback_t callback,
	     void* arg)
{
  create_item_t* ci = malloc (sizeof (create_item_t));
  memset (ci, 0, sizeof (create_item_t));
  ci->automaton = a;
  ci->text_bd = buffer_copy (text_bd);
  ci->retain_privilege = retain_privilege;
  ci->bda = buffer_copy (bda);
  ci->bdb = buffer_copy (bdb);
  ci->callback = callback;
  ci->arg = arg;

  *system->createq_tail = ci;
  system->createq_tail = &ci->next;
}

static void
destroy_create_item (create_item_t* ci)
{
  buffer_destroy (ci->text_bd);
  buffer_destroy (ci->bda);
  buffer_destroy (ci->bdb);
  free (ci);
}

static void
push_bind (system_t* system,
	   binding_t* b)
{
  bind_item_t* bi = malloc (sizeof (bind_item_t));
  memset (bi, 0, sizeof (bind_item_t));
  bi->binding = b;

  *system->bindq_tail = bi;
  system->bindq_tail = &bi->next;
}

static void
destroy_bind_item (bind_item_t* bi)
{
  free (bi);
}

static void
push_init (system_t* system,
	   automaton_t* a,
	   bd_t bda,
	   bd_t bdb)
{
  init_item_t* i = malloc (sizeof (init_item_t));
  memset (i, 0, sizeof (init_item_t));
  i->automaton = a;
  i->bda = buffer_copy (bda);
  i->bdb = buffer_copy (bdb);
  
  *system->initq_tail = i;
  system->initq_tail = &i->next;
}

static void
pop_init (system_t* system)
{
  init_item_t* i = system->initq_head;
  system->initq_head = i->next;
  if (system->initq_head == 0) {
    system->initq_tail = &system->initq_head;
  }

  buffer_destroy (i->bda);
  buffer_destroy (i->bdb);
  free (i);
}

/* static void */
/* automaton_create (system_t* system, */
/* 		  aid_t aid) */
/* { */
/*   automaton_t* a = malloc (sizeof (automaton_t)); */
/*   memset (a, 0, sizeof (automaton_t)); */
/*   a->aid = aid; */
/*   a->next = system->automaton_head; */
/*   system->automaton_head = a; */
/* } */

/* /\* static void *\/ */
/* /\* binding_bind (binding_t* b) *\/ */
/* /\* { *\/ */
/* /\*   if (b->output_automaton->aid != -1 && b->input_automaton->aid != -1) { *\/ */
/* /\*     /\\* Describe the output automaton. *\\/ *\/ */
/* /\*     description_t output_description; *\/ */
/* /\*     if (description_init (&output_description, b->output_automaton->aid) != 0) { *\/ */
/* /\*       b->error = lily_error; *\/ */
/* /\*       return; *\/ */
/* /\*     } *\/ */
    
/* /\*     /\\* Look up the actions. *\\/ *\/ */
/* /\*     action_desc_t output_action; *\/ */
/* /\*     if (b->output_action == -1) { *\/ */
/* /\*       if (description_read_name (&output_description, &output_action, b->output_action_begin, b->output_action_end) != 0) { *\/ */
/* /\* 	b->error = lily_error; *\/ */
/* /\* 	description_fini (&output_description); *\/ */
/* /\* 	return; *\/ */
/* /\*       } *\/ */
/* /\*     } *\/ */
/* /\*     else { *\/ */
/* /\*       if (description_read_number (&output_description, &output_action, b->output_action) != 0) { *\/ */
/* /\* 	b->error = lily_error; *\/ */
/* /\* 	description_fini (&output_description); *\/ */
/* /\* 	return; *\/ */
/* /\*       } *\/ */
/* /\*     } *\/ */
    
/* /\*     b->output_action = output_action.number; *\/ */
    
/* /\*     switch (output_action.parameter_mode) { *\/ */
/* /\*     case NO_PARAMETER: *\/ */
/* /\*       b->output_parameter = 0; *\/ */
/* /\*       break; *\/ */
/* /\*     case PARAMETER: *\/ */
/* /\*       /\\* Okay. *\\/ *\/ */
/* /\*       break; *\/ */
/* /\*     case AUTO_PARAMETER: *\/ */
/* /\*       b->output_parameter = b->input_automaton->aid; *\/ */
/* /\*       break; *\/ */
/* /\*     } *\/ */
      
/* /\*     description_fini (&output_description); *\/ */

/* /\*     /\\* Describe the input automaton. *\\/ *\/ */
/* /\*     description_t input_description; *\/ */
/* /\*     if (description_init (&input_description, b->input_automaton->aid) != 0) { *\/ */
/* /\*       b->error = lily_error; *\/ */
/* /\*       return; *\/ */
/* /\*     } *\/ */
    
/* /\*     /\\* Look up the actions. *\\/ *\/ */
/* /\*     action_desc_t input_action; *\/ */
/* /\*     if (b->input_action == -1) { *\/ */
/* /\*       if (description_read_name (&input_description, &input_action, b->input_action_begin, b->input_action_end) != 0) { *\/ */
/* /\* 	b->error = lily_error; *\/ */
/* /\* 	description_fini (&input_description); *\/ */
/* /\* 	return; *\/ */
/* /\*       } *\/ */
/* /\*     } *\/ */
/* /\*     else { *\/ */
/* /\*       if (description_read_number (&input_description, &input_action, b->input_action) != 0) { *\/ */
/* /\* 	b->error = lily_error; *\/ */
/* /\* 	description_fini (&input_description); *\/ */
/* /\* 	return; *\/ */
/* /\*       } *\/ */
/* /\*     } *\/ */
    
/* /\*     b->input_action = input_action.number; *\/ */
    
/* /\*     switch (input_action.parameter_mode) { *\/ */
/* /\*     case NO_PARAMETER: *\/ */
/* /\*       b->input_parameter = 0; *\/ */
/* /\*       break; *\/ */
/* /\*     case PARAMETER: *\/ */
/* /\*       /\\* Okay. *\\/ *\/ */
/* /\*       break; *\/ */
/* /\*     case AUTO_PARAMETER: *\/ */
/* /\*       b->input_parameter = b->output_automaton->aid; *\/ */
/* /\*       break; *\/ */
/* /\*     } *\/ */
    
/* /\*     description_fini (&input_description); *\/ */

/* /\*     push_bind (b); *\/ */
/* /\*   } *\/ */
/* /\* } *\/ */

/* /\* static binding_t* *\/ */
/* /\* create_binding (system_t* system, *\/ */
/* /\* 		automaton_t* output_automaton, *\/ */
/* /\* 		const char* output_action_begin, *\/ */
/* /\* 		const char* output_action_end, *\/ */
/* /\* 		ano_t output_action, *\/ */
/* /\* 		int output_parameter, *\/ */
/* /\* 		automaton_t* input_automaton, *\/ */
/* /\* 		const char* input_action_begin, *\/ */
/* /\* 		const char* input_action_end, *\/ */
/* /\* 		ano_t input_action, *\/ */
/* /\* 		int input_parameter) *\/ */
/* /\* { *\/ */
/* /\*   size_t size; *\/ */
/* /\*   binding_t* b = malloc (sizeof (binding_t)); *\/ */
/* /\*   memset (b, 0, sizeof (binding_t)); *\/ */
/* /\*   b->system = system; *\/ */
/* /\*   b->output_automaton = output_automaton; *\/ */
/* /\*   size = output_action_end - output_action_begin; *\/ */
/* /\*   b->output_action_begin = malloc (size); *\/ */
/* /\*   memcpy (b->output_action_begin, output_action_begin, size); *\/ */
/* /\*   b->output_action_end = b->output_action_begin + size; *\/ */
/* /\*   b->output_action = output_action; *\/ */
/* /\*   b->output_parameter = output_parameter; *\/ */
/* /\*   b->input_automaton = input_automaton; *\/ */
/* /\*   size = input_action_end - input_action_begin; *\/ */
/* /\*   b->input_action_begin = malloc (size); *\/ */
/* /\*   memcpy (b->input_action_begin, input_action_begin, size); *\/ */
/* /\*   b->input_action_end = b->input_action_begin + size; *\/ */
/* /\*   b->input_action = input_action; *\/ */
/* /\*   b->input_parameter = input_parameter; *\/ */

/* /\*   b->bid = -1; *\/ */

/* /\*   binding_bind (b); *\/ */

/* /\*   return b; *\/ */
/* /\* } *\/ */

/* /\* static void *\/ */
/* /\* binding_automaton_event (void* arg) *\/ */
/* /\* { *\/ */
/* /\*   binding_t* b = arg; *\/ */
/* /\*   binding_bind (b); *\/ */
/* /\* } *\/ */

/* /\* binding_t* *\/ */
/* /\* system_add_binding (system_t* system, *\/ */
/* /\* 		    automaton_t* output_automaton, *\/ */
/* /\* 		    const char* output_action_begin, *\/ */
/* /\* 		    const char* output_action_end, *\/ */
/* /\* 		    ano_t output_action, *\/ */
/* /\* 		    int output_parameter, *\/ */
/* /\* 		    automaton_t* input_automaton, *\/ */
/* /\* 		    const char* input_action_begin, *\/ */
/* /\* 		    const char* input_action_end, *\/ */
/* /\* 		    ano_t input_action, *\/ */
/* /\* 		    int input_parameter) *\/ */
/* /\* { *\/ */
/* /\*   if (output_action_begin != 0 && output_action_end == 0) { *\/ */
/* /\*     output_action_end = output_action_begin + strlen (output_action_begin); *\/ */
/* /\*   } *\/ */
/* /\*   if (input_action_begin != 0 && input_action_end == 0) { *\/ */
/* /\*     input_action_end = input_action_begin + strlen (input_action_begin); *\/ */
/* /\*   } *\/ */

/* /\*   binding_t* binding = create_binding (system, output_automaton, output_action_begin, output_action_end, output_action, output_parameter, input_automaton, input_action_begin, input_action_end, input_action, input_parameter); *\/ */

/* /\*   binding->next = system->binding_head; *\/ */
/* /\*   system->binding_head = binding; *\/ */

/* /\*   automaton_subscribe (output_automaton, binding_automaton_event, binding); *\/ */
/* /\*   automaton_subscribe (input_automaton, binding_automaton_event, binding); *\/ */

/* /\*   return binding; *\/ */
/* /\* } *\/ */

/* /\* static void *\/ */
/* /\* globbed_binding_bind (globbed_binding_t* b) *\/ */
/* /\* { *\/ */
/* /\*   if (b->output_automaton->aid != -1 && b->input_automaton->aid != -1 && (b->owner_automaton == 0 || b->owner_automaton->aid != -1)) { *\/ */
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
	      
/* /\* 	      binding_t* binding = create_binding (b->system, b->output_automaton, output_actions[out_idx].name_begin, output_actions[out_idx].name_end, output_actions[out_idx].number, output_param, b->input_automaton, input_actions[in_idx].name_begin, input_actions[in_idx].name_end, input_actions[in_idx].number, input_param, b->owner_automaton); *\/ */
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
/* /\* system_add_globbed_binding (system_t* system, *\/ */
/* /\* 			    automaton_t* output_automaton, *\/ */
/* /\* 			    const char* output_action_begin, *\/ */
/* /\* 			    const char* output_action_end, *\/ */
/* /\* 			    int output_parameter, *\/ */
/* /\* 			    automaton_t* input_automaton, *\/ */
/* /\* 			    const char* input_action_begin, *\/ */
/* /\* 			    const char* input_action_end, *\/ */
/* /\* 			    int input_parameter, *\/ */
/* /\* 			    automaton_t* owner_automaton) *\/ */
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

/* /\*   b->system = system; *\/ */
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
  
/* /\*   b->owner_automaton = owner_automaton; *\/ */

/* /\*   b->next = system->globbed_binding_head; *\/ */
/* /\*   system->globbed_binding_head = b; *\/ */

/* /\*   automaton_subscribe (output_automaton, globbed_binding_automaton_event, b); *\/ */
/* /\*   automaton_subscribe (input_automaton, globbed_binding_automaton_event, b); *\/ */
/* /\*   if (owner_automaton != 0) { *\/ */
/* /\*     automaton_subscribe (owner_automaton, globbed_binding_automaton_event, b); *\/ */
/* /\*   } *\/ */

/* /\*   globbed_binding_bind (b); *\/ */

/* /\*   return b; *\/ */
/* /\* } *\/ */

/* /\* bool *\/ */
/* /\* binding_bound (const binding_t* binding) *\/ */
/* /\* { *\/ */
/* /\*   return binding->bid != -1; *\/ */
/* /\* } *\/ */

automaton_t*
system_create (system_t* system,
	       bd_t text_bd,
	       bool retain_privilege,
	       bd_t bda,
	       bd_t bdb,
	       create_callback_t callback,
	       void* arg)
{
  automaton_t* a = automaton_create (system);
  push_create (system, a, text_bd, retain_privilege, bda, bdb, callback, arg);
  return a;
}

static binding_t*
system_bind (system_t* system,
	     const automaton_t* output_automaton,
	     const char* output_begin,
	     const char* output_end,
	     int output_parameter,
	     const automaton_t* input_automaton,
	     const char* input_begin,
	     const char* input_end,
	     int input_parameter)
{
  if (output_end == 0) {
    output_end = output_begin + strlen (output_begin);
  }
  if (input_end == 0) {
    input_end = input_begin + strlen (input_begin);
  }

  binding_t* b = malloc (sizeof (binding_t));
  memset (b, 0, sizeof (binding_t));
  b->bid = -1;
  b->output_automaton = output_automaton;
  size_t size = output_end - output_begin;
  b->output_begin = malloc (size);
  memcpy (b->output_begin, output_begin, size);
  b->output_end = b->output_begin + size;
  b->output_parameter = output_parameter;
  b->input_automaton = input_automaton;
  size = input_end - input_begin;
  b->input_begin = malloc (size);
  memcpy (b->input_begin, input_begin, size);
  b->input_end = b->input_begin + size;
  b->input_parameter = input_parameter;

  b->next = system->binding_head;
  system->binding_head = b;

  return b;
}

static bool
system_system_action_precondition (const system_t* system)
{
  return system->createq_head != 0 || system->bindq_head != 0;
}

void
system_system_action (system_t* system)
{
  if (system->createq_head != 0) {
    create_item_t* ci = system->createq_head;

    system->createq_head = 0;
    system->createq_tail = &system->createq_head;

    while (ci != 0) {
      automaton_t* a = ci->automaton;
      aid_t aid = create (ci->text_bd, ci->retain_privilege);
      if (aid != -1) {
	automaton_set_aid (a, aid, lily_error);

	if (automaton_interface_check (a, 0) == 0) {
	  /* This binding will be processed later. */
	  binding_t* b = system_bind (system,
				      system->this, SA_INIT_OUT_NAME, 0, aid,
				      a, SA_INIT_IN_NAME, 0, 0);
	  push_bind (system, b);
	  
	  push_init (system, a, ci->bda, ci->bdb);
	}
	else {
	  destroy (aid);
	  a->aid = -1;
	  a->error = LILY_ERROR_INVAL;
	}
      }
      
      if (ci->callback != 0) {
	ci->callback (ci->arg, a);
      }

      create_item_t* temp = ci;
      ci = temp->next;
      
      destroy_create_item (temp);
    }
  }

  if (system->bindq_head != 0) {
    bind_item_t* bi = system->bindq_head;

    system->bindq_head = 0;
    system->bindq_tail = &system->bindq_head;

    while (bi != 0) {
      binding_t* b = bi->binding;

      if (binding_resolve (b) == 0) {
	b->bid = bind (b->output_automaton->aid, b->output_action, b->output_parameter,
		       b->input_automaton->aid, b->input_action, b->input_parameter);
	b->error = lily_error;
      }

      /* TODO:  Tell everyone about the binding. */
      logs (__func__);

      bind_item_t* temp = bi;
      bi = temp->next;
      
      destroy_bind_item (temp);
    }

  }

  finish_internal ();
}

static bool
system_init_out_precondition (const system_t* system)
{
  return system->initq_head != 0;
}

void
system_init_out (system_t* system,
		 aid_t aid)
{
  if (system_init_out_precondition (system) && system->initq_head->automaton->aid == aid) {
    bd_t bda = -1;
    size_t bda_size = buffer_size (system->initq_head->bda);
    if (bda_size != -1) {
      bda = system->init_bda;
      buffer_assign (bda, system->initq_head->bda, 0, bda_size);
    }

    bd_t bdb = -1;
    size_t bdb_size = buffer_size (system->initq_head->bdb);
    if (bdb_size != -1) {
      bdb = system->init_bdb;
      buffer_assign (bdb, system->initq_head->bdb, 0, bdb_size);
    }

    pop_init (system);
    finish_output (true, bda, bdb);
  }

  finish_output (false, -1, -1);
}

void
system_schedule (const system_t* system)
{
  if (system_system_action_precondition (system)) {
    schedule (system->system_action, 0);
  }
  if (system_init_out_precondition (system)) {
    schedule (system->init_out, system->initq_head->automaton->aid);
  }
}
