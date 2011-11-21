#ifndef __binding_manager_hpp__
#define __binding_manager_hpp__

/*
  File
  ----
  binding_manager.hpp
  
  Description
  -----------
  Manage bindings between output actions and input actions.

  Authors:
  Justin R. Wilson
*/

#include "automaton.hpp"
#include <unordered_set>

struct output_action {
  automaton_interface* automaton;
  void* action_entry_point;
  parameter_t parameter;

  output_action (automaton_interface* a,
		 void* aep,
		 parameter_t p) :
    automaton (a),
    action_entry_point (aep),
    parameter (p)
  { }

  bool
  operator== (const output_action& other) const
  {
    return automaton == other.automaton && action_entry_point == other.action_entry_point && parameter == other.parameter;
  }
};

struct input_action {
  automaton_interface* automaton;
  void* action_entry_point;
  parameter_t parameter;

  input_action (automaton_interface* a,
		void* aep,
		parameter_t p) :
    automaton (a),
    action_entry_point (aep),
    parameter (p)
  { }

  bool
  operator== (const input_action& other) const
  {
    return automaton == other.automaton && action_entry_point == other.action_entry_point && parameter == other.parameter;
  }
};

class binding_manager {
public:
  typedef std::unordered_set<input_action> input_action_set_type;

private:
  typedef std::unordered_map<output_action, input_action_set_type> bindings_type;
  static bindings_type* bindings_;

public:
  static void
  initialize ();

  static void
  bind (automaton_interface* output_automaton,
	void* output_action_entry,
	parameter_t output_parameter,
	automaton_interface* input_automaton,
	void* input_action_entry,
	parameter_t input_parameter);
  
  static const input_action_set_type*
  get_bound_inputs (automaton_interface* output_automaton,
		    void* output_action_entry_point,
		    parameter_t output_parameter);
};

#endif /* __binding_manager_hpp__ */
