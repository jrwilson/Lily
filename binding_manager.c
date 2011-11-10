/*
  File
  ----
  binding_manager.c
  
  Description
  -----------
  Manage bindings between output actions and input actions.

  Authors:
  Justin R. Wilson
*/

#include "binding_manager.h"
#include "kassert.h"

typedef struct {
  automaton_t* automaton;
  void* action_entry_point;
  parameter_t parameter;
} output_action_t;

static list_allocator_t* list_allocator = 0;
/* The set of bindings. */
static hash_map_t* bindings = 0;

/* The hash map of bindings uses the output action as the key. */
static size_t
output_action_hash_func (const void* x)
{
  const output_action_t* ptr = static_cast<const output_action_t*> (x);
  return (size_t)ptr->automaton ^ (size_t)ptr->action_entry_point ^ ptr->parameter;
}

static int
output_action_compare_func (const void* x,
			    const void* y)
{
  const output_action_t* p1 = static_cast<const output_action_t*> (x);
  const output_action_t* p2 = static_cast<const output_action_t*> (y);
  if (p1->automaton != p2->automaton) {
    return p1->automaton - p2->automaton;
  }
  else if (p1->action_entry_point != p2->action_entry_point) {
    return static_cast<uint8_t*> (p1->action_entry_point) - static_cast<uint8_t*> (p2->action_entry_point);
  }
  else {
    return p1->parameter - p2->parameter;
  }
}

void
binding_manager_initialize (list_allocator_t* la)
{
  kassert (la != 0);
  kassert (bindings == 0);

  list_allocator = la;
  bindings = hash_map_allocate (list_allocator, output_action_hash_func, output_action_compare_func);
  kassert (bindings != 0);
}

static output_action_t*
allocate_output_action (automaton_t* automaton,
			void* action_entry_point,
			parameter_t parameter)
{
  output_action_t* ptr = static_cast<output_action_t*> (list_allocator_alloc (list_allocator, sizeof (output_action_t)));
  kassert (ptr != 0);
  ptr->automaton = automaton;
  ptr->action_entry_point = action_entry_point;
  ptr->parameter = parameter;
  return ptr;
}

static input_action_t*
allocate_input_action (automaton_t* automaton,
		       void* action_entry_point,
		       parameter_t parameter)
{
  input_action_t* ptr = static_cast<input_action_t*> (list_allocator_alloc (list_allocator, sizeof (input_action_t)));
  kassert (ptr != 0);
  ptr->automaton = automaton;
  ptr->action_entry_point = action_entry_point;
  ptr->parameter = parameter;
  ptr->next = 0;
  return ptr;
}

void
binding_manager_bind (automaton_t* output_automaton,
		      void* output_action_entry_point,
		      parameter_t output_parameter,
		      automaton_t* input_automaton,
		      void* input_action_entry_point,
		      parameter_t input_parameter)
{
  kassert (bindings != 0);
  kassert (output_automaton != 0);
  kassert (automaton_get_action_type (output_automaton, output_action_entry_point) == OUTPUT);
  kassert (input_automaton != 0);
  kassert (automaton_get_action_type (input_automaton, input_action_entry_point) == INPUT);

  /* TODO:  All of the bind checks. */

  output_action_t action;
  action.automaton = output_automaton;
  action.action_entry_point = output_action_entry_point;
  action.parameter = output_parameter;

  output_action_t* output_action = &action;

  if (!hash_map_contains (bindings, &output_action)) {
    output_action = allocate_output_action (output_automaton, output_action_entry_point, output_parameter);
    hash_map_insert (bindings, output_action, 0);
  }

  input_action_t* input_action = allocate_input_action (input_automaton, input_action_entry_point, input_parameter);
  input_action->next = static_cast<input_action_t*> (hash_map_find (bindings, output_action));
  hash_map_erase (bindings, output_action);
  hash_map_insert (bindings, output_action, input_action);
}

input_action_t*
binding_manager_get_bound_inputs (automaton_t* output_automaton,
				  void* output_action_entry_point,
				  parameter_t output_parameter)
{
  kassert (bindings != 0);
  
  output_action_t output_action;
  output_action.automaton = output_automaton;
  output_action.action_entry_point = output_action_entry_point;
  output_action.parameter = output_parameter;
  return static_cast<input_action_t*> (hash_map_find (bindings, &output_action));
}
