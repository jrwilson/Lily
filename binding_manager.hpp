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
    ::automaton* const automaton;
    const void* const action_entry_point;
    parameter_mode_t parameter_mode;
    aid_t const parameter;

    output_action (::automaton* const a,
		   const void* aep,
		   parameter_mode_t pm,
		   aid_t p) :
      automaton (a),
      action_entry_point (aep),
      parameter_mode (pm),
      parameter (p)
    { }

    bool
    operator== (const output_action& other) const
    {
      return automaton == other.automaton && action_entry_point == other.action_entry_point && parameter_mode == other.parameter_mode && parameter == other.parameter;
    }
  };

  struct input_action {
    ::automaton* const automaton;
    const void* const action_entry_point;
    parameter_mode_t parameter_mode;
    aid_t const parameter;

    input_action (::automaton* const a,
		  const void* aep,
		  parameter_mode_t pm,
		  aid_t p) :
      automaton (a),
      action_entry_point (aep),
      parameter_mode (pm),
      parameter (p)
    { }

    bool
    operator== (const input_action& other) const
    {
      return automaton == other.automaton && action_entry_point == other.action_entry_point && parameter_mode == other.parameter_mode && parameter == other.parameter;
    }
  };

public:
  typedef std::unordered_set<input_action, std::hash<input_action>, std::equal_to<input_action>, system_allocator<input_action> > input_action_set_type;

private:
  typedef std::unordered_map<output_action, input_action_set_type, std::hash<output_action>, std::equal_to<output_action>, system_allocator<std::pair<const output_action, input_action_set_type> > > bindings_type;
  static bindings_type bindings_;

  void
  bind_ (automaton* output_automaton,
	 const void* output_action_entry_point,
	 parameter_mode_t output_parameter_mode,
	 aid_t output_parameter,
	 automaton* input_automaton,
	 const void* input_action_entry_point,
	 parameter_mode_t input_parameter_mode,
	 aid_t input_parameter)
  {
    // Check the output action dynamically.
    kassert (output_automaton != 0);
    automaton::const_action_iterator output_pos = output_automaton->action_find (output_action_entry_point);
    kassert (output_pos != output_automaton->action_end () && output_pos->type == OUTPUT);

    // Check the input action dynamically.
    kassert (input_automaton != 0);
    automaton::const_action_iterator input_pos = input_automaton->action_find (input_action_entry_point);
    kassert (input_pos != input_automaton->action_end () && input_pos->type == INPUT);

    // TODO:  All of the bind checks.
    output_action oa (output_automaton, output_action_entry_point, output_parameter_mode, output_parameter);
    input_action ia (input_automaton, input_action_entry_point, input_parameter_mode, input_parameter);
    
    std::pair<bindings_type::iterator, bool> r = bindings_.insert (std::make_pair (oa, input_action_set_type ()));
    r.first->second.insert (ia);
  }

public:
  template <class OutputAction, class InputAction>
  void
  bind (automaton* output_automaton,
	void (*output_ptr) (void),
	automaton* input_automaton,
	void (*input_ptr) (void))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value && OutputAction::parameter_mode == NO_PARAMETER);
    STATIC_ASSERT (is_input_action<InputAction>::value && InputAction::parameter_mode == NO_PARAMETER);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::value_type COMMA typename InputAction::value_type>::value);

    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), OutputAction::parameter_mode, 0,
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, 0);
  }

  template <class OutputAction, class InputAction>
  void
  bind (automaton* output_automaton,
	void (*output_ptr) (void),
	automaton* input_automaton,
	void (*input_ptr) (typename InputAction::parameter_type),
	typename InputAction::parameter_type input_parameter)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value && OutputAction::parameter_mode == NO_PARAMETER);
    STATIC_ASSERT (is_input_action<InputAction>::value && (InputAction::parameter_mode == PARAMETER || InputAction::parameter_mode == AUTO_PARAMETER));
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::value_type COMMA typename InputAction::value_type>::value);

    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), OutputAction::parameter_mode, 0,
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, aid_cast (input_parameter));
  }

  template <class OutputAction, class InputAction>
  void
  bind (automaton* output_automaton,
	void (*output_ptr) (void),
	automaton* input_automaton,
	void (*input_ptr) (typename InputAction::value_type))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value && OutputAction::parameter_mode == NO_PARAMETER);
    STATIC_ASSERT (is_input_action<InputAction>::value && InputAction::parameter_mode == NO_PARAMETER);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::value_type COMMA typename InputAction::value_type>::value);

    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), OutputAction::parameter_mode, 0,
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, 0);
  }

  template <class OutputAction, class InputAction>
  void
  bind (automaton* output_automaton,
	void (*output_ptr) (void),
	automaton* input_automaton,
	void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::value_type),
	typename InputAction::parameter_type input_parameter)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value && OutputAction::parameter_mode == NO_PARAMETER);
    STATIC_ASSERT (is_input_action<InputAction>::value && (InputAction::parameter_mode == PARAMETER || InputAction::parameter_mode == AUTO_PARAMETER));
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::value_type COMMA typename InputAction::value_type>::value);

    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), OutputAction::parameter_mode, 0,
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, aid_cast (input_parameter));
  }

  template <class OutputAction, class InputAction>
  void
  bind (automaton* output_automaton,
	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
	automaton* input_automaton,
	void (*input_ptr) (void))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value && (OutputAction::parameter_mode == PARAMETER || OutputAction::parameter_mode == AUTO_PARAMETER));
    STATIC_ASSERT (is_input_action<InputAction>::value && InputAction::parameter_mode == NO_PARAMETER);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::value_type COMMA typename InputAction::value_type>::value);

    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), OutputAction::parameter_mode, aid_cast (output_parameter),
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, 0);
  }

  template <class OutputAction, class InputAction>
  void
  bind (automaton* output_automaton,
	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
	automaton* input_automaton,
	void (*input_ptr) (typename InputAction::parameter_type),
	typename InputAction::parameter_type input_parameter)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value && (OutputAction::parameter_mode == PARAMETER || OutputAction::parameter_mode == AUTO_PARAMETER));
    STATIC_ASSERT (is_input_action<InputAction>::value && (InputAction::parameter_mode == PARAMETER || InputAction::parameter_mode ==  AUTO_PARAMETER));
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::value_type COMMA typename InputAction::value_type>::value);

    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), OutputAction::parameter_mode, aid_cast (output_parameter),
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, aid_cast(input_parameter));
  }

  template <class OutputAction, class InputAction>
  void
  bind (automaton* output_automaton,
	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
	automaton* input_automaton,
	void (*input_ptr) (typename InputAction::value_type))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value && (OutputAction::parameter_mode == PARAMETER || OutputAction::parameter_mode == AUTO_PARAMETER));
    STATIC_ASSERT (is_input_action<InputAction>::value && InputAction::parameter_mode == NO_PARAMETER);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::value_type COMMA typename InputAction::value_type>::value);

    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), OutputAction::parameter_mode, aid_cast (output_parameter),
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, 0);
  }

  template <class OutputAction, class InputAction>
  void
  bind (automaton* output_automaton,
	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
	automaton* input_automaton,
	void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::value_type),
	typename InputAction::parameter_type input_parameter)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value && (OutputAction::parameter_mode == PARAMETER || OutputAction::parameter_mode == AUTO_PARAMETER));
    STATIC_ASSERT (is_input_action<InputAction>::value && (InputAction::parameter_mode == PARAMETER || InputAction::parameter_mode ==  AUTO_PARAMETER));
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::value_type COMMA typename InputAction::value_type>::value);

    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), OutputAction::parameter_mode, aid_cast (output_parameter),
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, aid_cast(input_parameter));
  }
  
  static const binding_manager::input_action_set_type*
  get_bound_inputs (automaton* automaton,
		    const void* action_entry_point,
		    parameter_mode_t parameter_mode,
		    aid_t parameter)
  {
    output_action oa (automaton, action_entry_point, parameter_mode, parameter);
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
