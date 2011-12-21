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

#include "kassert.hpp"

class automaton;
class buffer;

class rts {
public:
  typedef std::unordered_set<caction, std::hash<caction>, std::equal_to<caction>, system_allocator<caction> > input_action_set_type;

  static automaton*
  create (frame_t frame);

  template <class OutputAction,
	    class InputAction>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (void))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (void))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (IT1))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (IT1))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1,
	    class IT2>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (IT1, IT2))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1,
	    class IT2>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (IT1, IT2))
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type),
	typename InputAction::parameter_type input_parameter)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type),
	typename InputAction::parameter_type input_parameter)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type, IT1),
	typename InputAction::parameter_type input_parameter)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type, IT1),
	typename InputAction::parameter_type input_parameter)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1,
	    class IT2>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type, IT1, IT2),
	typename InputAction::parameter_type input_parameter)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1,
	    class IT2>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type, IT1, IT2),
	typename InputAction::parameter_type input_parameter)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size);
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
  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (void),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (void),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>  
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (void),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::copy_value_type),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::copy_value_type),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::copy_value_type),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t, typename InputAction::copy_value_type),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t, typename InputAction::copy_value_type),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t, typename InputAction::copy_value_type),
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size);
  }
  
  static void
  bind_ (automaton* output_automaton,
	 const void* output_action_entry_point,
	 parameter_mode_t output_parameter_mode,
	 aid_t output_parameter,
	 automaton* input_automaton,
	 const void* input_action_entry_point,
	 parameter_mode_t input_parameter_mode,
	 aid_t input_parameter,
	 buffer_value_mode_t buffer_value_mode,
	 copy_value_mode_t copy_value_mode,
	 size_t copy_value_size);

  static aid_t current_aid_;
  typedef std::unordered_map<aid_t, automaton*, std::hash<aid_t>, std::equal_to<aid_t>, system_allocator<std::pair<const aid_t, automaton*> > > aid_map_type;
  static aid_map_type aid_map_;

  typedef std::unordered_map<caction, input_action_set_type, std::hash<caction>, std::equal_to<caction>, system_allocator<std::pair<const caction, input_action_set_type> > > bindings_type;
  static bindings_type bindings_;
};

#endif /* __rts_hpp__ */
