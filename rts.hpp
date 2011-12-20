#ifndef __rts_hpp__
#define __rts_hpp__

/*
  File
  ----
  rts.hpp
  
  Description
  -----------
  The run-time system (rts).

  Authors:
  Justin R. Wilson
*/

#include "aid.hpp"
#include "bid.hpp"
#include "descriptor.hpp"
#include "vm_def.hpp"
#include <unordered_set>
#include <unordered_map>
#include "system_allocator.hpp"
#include "action.hpp"

class automaton;
class buffer;

class rts {
public:
  typedef std::unordered_set<caction, std::hash<caction>, std::equal_to<caction>, system_allocator<caction> > input_action_set_type;

  static automaton*
  create (frame_t frame);

  template <class OutputAction, class InputAction>
  static void
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
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, 0, OutputAction::value_size);
  }

  template <class OutputAction, class InputAction>
  static void
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
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, aid_cast (input_parameter), OutputAction::value_size);
  }

  template <class OutputAction, class InputAction>
  static void
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
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, 0, OutputAction::value_size);
  }

  template <class OutputAction, class InputAction>
  static void
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
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, aid_cast (input_parameter), OutputAction::value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind (automaton* output_automaton,
	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
	automaton* input_automaton,
	void (*input_ptr) (void))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value && (OutputAction::parameter_mode == PARAMETER || OutputAction::parameter_mode == AUTO_PARAMETER));
    STATIC_ASSERT (is_input_action<InputAction>::value && InputAction::parameter_mode == NO_PARAMETER);
    // Value types must be the same.  This implies that the value sizes are the same.
    STATIC_ASSERT (std::is_same <typename OutputAction::value_type COMMA typename InputAction::value_type>::value);

    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), OutputAction::parameter_mode, aid_cast (output_parameter),
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, 0, OutputAction::value_size);
  }

  template <class OutputAction, class InputAction>
  static void
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
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, aid_cast(input_parameter), OutputAction::value_size);
  }

  template <class OutputAction, class InputAction>
  static void
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
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, 0, OutputAction::value_size);
  }

  template <class OutputAction, class InputAction>
  static void
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
	   input_automaton, reinterpret_cast<const void*> (input_ptr), InputAction::parameter_mode, aid_cast(input_parameter), OutputAction::value_size);
  }

  static inline const input_action_set_type*
  get_bound_inputs (const caction& output_action)
  {
    bindings_type::const_iterator pos = bindings_.find (output_action);
    if (pos != bindings_.end ()) {
      return &pos->second;
    }
    else {
      return 0;
    }
  }

private:
  static void
  bind_ (automaton* output_automaton,
	 const void* output_action_entry_point,
	 parameter_mode_t output_parameter_mode,
	 aid_t output_parameter,
	 automaton* input_automaton,
	 const void* input_action_entry_point,
	 parameter_mode_t input_parameter_mode,
	 aid_t input_parameter,
	 size_t value_size);

  static aid_t current_aid_;
  typedef std::unordered_map<aid_t, automaton*, std::hash<aid_t>, std::equal_to<aid_t>, system_allocator<std::pair<const aid_t, automaton*> > > aid_map_type;
  static aid_map_type aid_map_;

  typedef std::unordered_map<caction, input_action_set_type, std::hash<caction>, std::equal_to<caction>, system_allocator<std::pair<const caction, input_action_set_type> > > bindings_type;
  static bindings_type bindings_;
};

#endif /* __rts_hpp__ */
