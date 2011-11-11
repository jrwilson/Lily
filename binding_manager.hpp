#ifndef __binding_manager_h__
#define __binding_manager_h__

/*
  File
  ----
  binding_manager.h
  
  Description
  -----------
  Manage bindings between output actions and input actions.

  Authors:
  Justin R. Wilson
*/

#include "automaton.hpp"

typedef struct input_action input_action_t;
struct input_action {
  automaton_interface* automaton_;
  void* action_entry_point;
  parameter_t parameter;
  input_action_t* next;
};

void
binding_manager_initialize (list_allocator_t* list_allocator);

void
binding_manager_bind (automaton_interface* output_automaton,
		      void* output_action_entry,
		      parameter_t output_parameter,
		      automaton_interface* input_automaton,
		      void* input_action_entry,
		      parameter_t input_parameter);

input_action_t*
binding_manager_get_bound_inputs (automaton_interface* output_automaton,
				  void* output_action_entry_point,
				  parameter_t output_parameter);

#endif /* __binding_manager_h__ */
