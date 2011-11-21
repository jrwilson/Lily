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

#include "binding_manager.hpp"
#include "kassert.hpp"
#include "automaton_interface.hpp"

binding_manager::bindings_type* binding_manager::bindings_ = 0;

void
binding_manager::initialize ()
{
  kassert (bindings_ == 0);
  bindings_ = new bindings_type ();
}

void
binding_manager::bind (automaton_interface* output_automaton,
		       void* output_action_entry_point,
		       parameter_t output_parameter,
		       automaton_interface* input_automaton,
		       void* input_action_entry_point,
		       parameter_t input_parameter)
{
  kassert (bindings_ != 0);
  kassert (output_automaton != 0);
  kassert (output_automaton->get_action_type (output_action_entry_point) == OUTPUT);
  kassert (input_automaton != 0);
  kassert (input_automaton->get_action_type (input_action_entry_point) == INPUT);

  /* TODO:  All of the bind checks. */

  output_action oa (output_automaton, output_action_entry_point, output_parameter);
  input_action ia (input_automaton, input_action_entry_point, input_parameter);

  std::pair<bindings_type::iterator, bool> r = bindings_->insert (std::make_pair (oa, input_action_set_type ()));
  r.first->second.insert (ia);
}

const binding_manager::input_action_set_type*
binding_manager::get_bound_inputs (automaton_interface* output_automaton,
				   void* output_action_entry_point,
				   parameter_t output_parameter)
{
  kassert (bindings_ != 0);
  
  output_action oa (output_automaton, output_action_entry_point, output_parameter);
  bindings_type::const_iterator pos = bindings_->find (oa);
  if (pos != bindings_->end ()) {
    return &pos->second;
  }
  else {
    return 0;
  }
}
