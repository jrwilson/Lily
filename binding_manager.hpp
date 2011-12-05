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

#include <unordered_set>
#include <unordered_map>
#include "automaton.hpp"

class binding_manager {
private:
  struct output_action {
    ::automaton* automaton;
    output_func output;
    void* parameter;

    output_action (::automaton* a,
		   output_func o,
		   void* p) :
      automaton (a),
      output (o),
      parameter (p)
    { }

    bool
    operator== (const output_action& other) const
    {
      return automaton == other.automaton && output == other.output && parameter == other.parameter;
    }
  };

  struct input_action {
    ::automaton* automaton;
    input_func input;
    void* parameter;

    input_action (::automaton* a,
		   input_func i,
		   void* p) :
      automaton (a),
      input (i),
      parameter (p)
    { }

    bool
    operator== (const input_action& other) const
    {
      return automaton == other.automaton && input == other.input && parameter == other.parameter;
    }
  };

public:
  typedef std::unordered_set<input_action, std::hash<input_action>, std::equal_to<input_action>, list_allocator<input_action> > input_action_set_type;

private:
  list_alloc& alloc_;
  typedef std::unordered_map<output_action, input_action_set_type, std::hash<output_action>, std::equal_to<output_action>, list_allocator<std::pair<const output_action, input_action_set_type> > > bindings_type;
  bindings_type bindings_;

  template <class OutputAction, class InputAction>
  void
  bind_ (automaton* output_automaton,
	 typename OutputAction::parameter_type output_parameter,
	 output_action_tag,
	 automaton* input_automaton,
	 typename InputAction::parameter_type input_parameter,
	 input_action_tag)
  {
    /* TODO:  All of the bind checks. */
    output_action oa (output_automaton, reinterpret_cast<output_func> (&OutputAction::action), output_parameter);
    input_action ia (input_automaton, reinterpret_cast<input_func> (&InputAction::action), input_parameter);
    
    std::pair<bindings_type::iterator, bool> r = bindings_.insert (std::make_pair (oa, input_action_set_type (3, input_action_set_type::hasher (), input_action_set_type::key_equal (), input_action_set_type::allocator_type (alloc_))));
    r.first->second.insert (ia);
  }

public:
  binding_manager (list_alloc& a) :
    alloc_ (a),
    bindings_ (3, bindings_type::hasher (), bindings_type::key_equal (), bindings_type::allocator_type (a))
  { }

  template <class OutputAction, class InputAction>
  void
  bind (automaton* output_automaton,
	const OutputAction&,
	typename OutputAction::parameter_type output_parameter,
	automaton* input_automaton,
	const InputAction&,
	typename InputAction::parameter_type input_parameter)
  {
    kassert (output_automaton != 0);
    kassert (input_automaton != 0);

    bind_<OutputAction, InputAction> (output_automaton, output_parameter, typename OutputAction::action_category (),
				      input_automaton, input_parameter, typename InputAction::action_category ());
  }
  
  const binding_manager::input_action_set_type*
  get_bound_inputs (automaton* output_automaton,
		    output_func output_action_entry_point,
		    void* output_parameter)
  {
    output_action oa (output_automaton, output_action_entry_point, output_parameter);
    bindings_type::const_iterator pos = bindings_.find (oa);
    if (pos != bindings_.end ()) {
      return &pos->second;
    }
    else {
      return 0;
    }
  }
  
};

#endif /* __binding_manager_hpp__ */
