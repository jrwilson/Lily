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

template <class Alloc, template <typename> class Allocator>
class binding_manager {
private:
  struct output_action {
    ::automaton<Alloc, Allocator>* const automaton;
    size_t const action_entry_point;
    const void* const parameter;

    output_action (::automaton<Alloc, Allocator>* const a,
		   size_t const aep,
		   const void* const p) :
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
    ::automaton<Alloc, Allocator>* const automaton;
    size_t const action_entry_point;
    const void* const parameter;

    input_action (::automaton<Alloc, Allocator>* const a,
		  size_t const aep,
		  const void* const p) :
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

public:
  typedef std::unordered_set<input_action, std::hash<input_action>, std::equal_to<input_action>, Allocator<input_action> > input_action_set_type;

private:
  Alloc& alloc_;
  typedef std::unordered_map<output_action, input_action_set_type, std::hash<output_action>, std::equal_to<output_action>, Allocator<std::pair<const output_action, input_action_set_type> > > bindings_type;
  bindings_type bindings_;

public:
  binding_manager (Alloc& a) :
    alloc_ (a),
    bindings_ (3, typename bindings_type::hasher (), typename bindings_type::key_equal (), typename bindings_type::allocator_type (a))
  { }

  template <class OutputAction, class InputAction>
  void
  bind (automaton<Alloc, Allocator>* output_automaton,
	typename OutputAction::parameter_type output_parameter,
	automaton<Alloc, Allocator>* input_automaton,
	typename InputAction::parameter_type input_parameter)
  {
    kassert (output_automaton != 0);
    // This checks that the output is an output at compile time.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    // This checks that the automaton contains the action at run time.
    kassert (output_automaton->get_action (OutputAction::action_entry_point).type == automaton<Alloc COMMA Allocator>::OUTPUT);
    kassert (input_automaton != 0);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Message types must be the same.
    kassert (input_automaton->get_action (InputAction::action_entry_point).type == automaton<Alloc COMMA Allocator>::INPUT);
    
    STATIC_ASSERT (std::is_same <typename OutputAction::message_type COMMA typename InputAction::message_type>::value);

    // /* TODO:  All of the bind checks. */
    output_action oa (output_automaton, OutputAction::action_entry_point, reinterpret_cast<const void*> (output_parameter));
    input_action ia (input_automaton, InputAction::action_entry_point, reinterpret_cast<const void*> (input_parameter));
    
    std::pair<typename bindings_type::iterator, bool> r = bindings_.insert (std::make_pair (oa, input_action_set_type (3, typename input_action_set_type::hasher (), typename input_action_set_type::key_equal (), typename input_action_set_type::allocator_type (alloc_))));
    r.first->second.insert (ia);
  }
  
  const binding_manager::input_action_set_type*
  get_bound_inputs (automaton<Alloc, Allocator>* output_automaton,
		    size_t output_action_entry_point,
		    const void* output_parameter)
  {
    output_action oa (output_automaton, output_action_entry_point, output_parameter);
    typename bindings_type::const_iterator pos = bindings_.find (oa);
    if (pos != bindings_.end ()) {
      return &pos->second;
    }
    else {
      return 0;
    }
  }
  
};

#endif /* __binding_manager_hpp__ */
