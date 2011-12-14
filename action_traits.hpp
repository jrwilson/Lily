#ifndef __action_traits_hpp__
#define __action_traits_hpp__

/*
  File
  ----
  action_traits.hpp
  
  Description
  -----------
  Types for describing actions.
  Algorithms for executing actions.

  Authors:
  Justin R. Wilson
*/

#include <type_traits>
#include "static_assert.hpp"
#include "aid.hpp"
#include <stddef.h>

// Size of the temporary buffer used to store the values produced by output actions.
const size_t VALUE_BUFFER_SIZE = 512;

// The absence of a type.
struct null_type { };

// Parameters.
struct no_parameter_tag { };
struct parameter_tag { };
struct auto_parameter_tag { };

enum parameter_mode_t {
  NO_PARAMETER,
  PARAMETER,
  AUTO_PARAMETER,
};

struct no_parameter {
  typedef no_parameter_tag parameter_category;
  typedef null_type parameter_type;
  static const parameter_mode_t parameter_mode = NO_PARAMETER;
};

template <class Parameter>
struct parameter {
  typedef parameter_tag parameter_category;
  typedef Parameter parameter_type;
  static const parameter_mode_t parameter_mode = PARAMETER;
};

struct auto_parameter {
  typedef auto_parameter_tag parameter_category;
  typedef aid_t parameter_type;
  static const parameter_mode_t parameter_mode = AUTO_PARAMETER;
};

// Values.
struct no_value_tag { };
struct value_tag { };

struct no_value {
  typedef no_value_tag value_category;
  typedef null_type value_type;
  static const size_t value_size = 0;
};

template <class Value>
struct value {
  typedef value_tag value_category;
  typedef Value value_type;
  static const size_t value_size = sizeof (Value);
};

// Actions.
struct input_action_tag { };
struct output_action_tag { };
struct internal_action_tag { };

enum action_type_t {
  INPUT,
  OUTPUT,
  INTERNAL,
};

struct input_action {
  typedef input_action_tag action_category;
  static const action_type_t action_type = INPUT;
};

struct output_action {
  typedef output_action_tag action_category;
  static const action_type_t action_type = OUTPUT;
};

struct internal_action {
  typedef internal_action_tag action_category;
  static const action_type_t action_type = INTERNAL;
};

// Put it all together.
struct up_uv_input_action_traits : public no_parameter, public no_value, public input_action { };

template <class Value>
struct up_v_input_action_traits : public no_parameter, public value<Value>, public input_action  { };

template <class Parameter>
struct p_uv_input_action_traits : public parameter<Parameter>, public no_value, public input_action  { };

template <class Parameter,
	  class Value>
struct p_v_input_action_traits : public parameter<Parameter>, public value<Value>, public input_action  { };

struct ap_uv_input_action_traits : public auto_parameter, public no_value, public input_action  { };

template <class Value>
struct ap_v_input_action_traits : public auto_parameter, public value<Value>, public input_action  { };

struct up_uv_output_action_traits : public no_parameter, public no_value, public output_action { };

template <class Value>
struct up_v_output_action_traits : public no_parameter, public value<Value>, public output_action  { };

template <class Parameter>
struct p_uv_output_action_traits : public parameter<Parameter>, public no_value, public output_action  { };

template <class Parameter,
	  class Value>
struct p_v_output_action_traits : public parameter<Parameter>, public value<Value>, public output_action  { };

struct ap_uv_output_action_traits : public auto_parameter, public no_value, public output_action  { };

template <class Value>
struct ap_v_output_action_traits : public auto_parameter, public value<Value>, public output_action  { };

struct up_internal_action_traits : public no_parameter, public no_value, public internal_action { };

template <class Parameter>
struct p_internal_action_traits : public parameter<Parameter>, public no_value, public internal_action  { };

// These struct encode the schema of action traits.
template <bool>
struct bool_dispatch : public std::false_type { };

template <>
struct bool_dispatch<true> : public std::true_type { };

template <class T>
struct is_parameter : public bool_dispatch<
  ((std::is_same<typename T::parameter_category, no_parameter_tag>::value && std::is_same<typename T::parameter_type, null_type>::value && T::parameter_mode == NO_PARAMETER) ||
   (std::is_same<typename T::parameter_category, parameter_tag>::value && sizeof (typename T::parameter_type) == sizeof (aid_t) && T::parameter_mode == PARAMETER) ||
   (std::is_same<typename T::parameter_category, auto_parameter_tag>::value && std::is_same<typename T::parameter_type, aid_t>::value && T::parameter_mode == AUTO_PARAMETER))
  > { };

template <class T>
struct is_value : public bool_dispatch<
  ((std::is_same<typename T::value_category, no_value_tag>::value && std::is_same<typename T::value_type, null_type>::value) ||
   (std::is_same<typename T::value_category, value_tag>::value && sizeof (typename T::value_type) <= VALUE_BUFFER_SIZE))
  > { };

template <class T>
struct is_input_action : public bool_dispatch<
  is_parameter<T>::value &&
  is_value<T>::value &&
  std::is_same<typename T::action_category, input_action_tag>::value && T::action_type == INPUT> { };

template <class T>
struct is_output_action : public bool_dispatch<
  is_parameter<T>::value &&
  is_value<T>::value &&
  std::is_same<typename T::action_category, output_action_tag>::value && T::action_type == OUTPUT> { };

template <class T>
struct is_internal_action : public bool_dispatch<
  is_parameter<T>::value &&
  std::is_same<typename T::action_category, internal_action_tag>::value && T::action_type == INTERNAL> { };

template <class T>
struct is_local_action : public bool_dispatch<is_output_action<T>::value || is_internal_action<T>::value> { };

template <class T>
struct is_action : public bool_dispatch<is_input_action<T>::value || is_output_action<T>::value || is_internal_action<T>::value> { };

// Algorithms for executing actions.
template <class InputAction,
	  class Effect,
	  class Schedule,
	  class Finish>
void
input_action (Effect effect,
	      Schedule schedule,
	      Finish finish)
{
  STATIC_ASSERT (is_input_action<InputAction>::value);
  STATIC_ASSERT (InputAction::parameter_mode == NO_PARAMETER && InputAction::value_size == 0);

  effect ();
  schedule ();
  finish ();
}

template <class InputAction,
	  class Effect,
	  class Schedule,
	  class Finish>
void
input_action (typename InputAction::parameter_type parameter,
	      Effect effect,
	      Schedule schedule,
	      Finish finish)
{
  STATIC_ASSERT (is_input_action<InputAction>::value);
  STATIC_ASSERT ((InputAction::parameter_mode == PARAMETER || InputAction::parameter_mode == AUTO_PARAMETER) && InputAction::value_size == 0);

  effect (parameter);
  schedule ();
  finish ();
}

template <class InputAction,
	  class Effect,
	  class Schedule,
	  class Finish>
void
input_action (typename InputAction::value_type& value,
	      Effect effect,
	      Schedule schedule,
	      Finish finish)
{
  STATIC_ASSERT (is_input_action<InputAction>::value);
  STATIC_ASSERT (InputAction::parameter_mode == NO_PARAMETER && InputAction::value_size != 0);

  effect (value);
  schedule ();
  finish ();
}

template <class InputAction,
	  class Effect,
	  class Schedule,
	  class Finish>
void
input_action (typename InputAction::parameter_type parameter,
	      typename InputAction::value_type& value,
	      Effect effect,
	      Schedule schedule,
	      Finish finish)
{
  STATIC_ASSERT (is_input_action<InputAction>::value);
  STATIC_ASSERT ((InputAction::parameter_mode == PARAMETER || InputAction::parameter_mode == AUTO_PARAMETER) && InputAction::value_size != 0);

  effect (parameter, value);
  schedule ();
  finish ();
}

template <class OutputAction,
	  class Remove,
	  class Precondition,
	  class Effect,
	  class Schedule,
	  class Finish>
void
output_action_ (no_value_tag,
		Remove remove,
		Precondition precondition,
		Effect effect,
		Schedule schedule,
		Finish finish)
{
  remove ();
  if (precondition ()) {
    effect ();
    schedule ();
    finish (true);
  }
  else {
    schedule ();
    finish (false);
  }
}

template <class OutputAction,
	  class Remove,
	  class Precondition,
	  class Effect,
	  class Schedule,
	  class Finish>
void
output_action_ (no_value_tag,
		typename OutputAction::parameter_type parameter,
		Remove remove,
		Precondition precondition,
		Effect effect,
		Schedule schedule,
		Finish finish)
{
  remove (parameter);
  if (precondition (parameter)) {
    effect (parameter);
    schedule ();
    finish (true);
  }
  else {
    schedule ();
    finish (false);
  }
}

template <class OutputAction,
	  class Remove,
	  class Precondition,
	  class Effect,
	  class Schedule,
	  class Finish>
void
output_action_ (value_tag,
		Remove remove,
		Precondition precondition,
		Effect effect,
		Schedule schedule,
		Finish finish)
{
  static typename OutputAction::value_type value;

  remove ();
  if (precondition ()) {
    effect (&value);
    schedule ();
    finish (&value);
  }
  else {
    schedule ();
    finish (false);
  }
}

template <class OutputAction,
	  class Remove,
	  class Precondition,
	  class Effect,
	  class Schedule,
	  class Finish>
void
output_action_ (value_tag,
		typename OutputAction::parameter_type parameter,
		Remove remove,
		Precondition precondition,
		Effect effect,
		Schedule schedule,
		Finish finish)
{
  static typename OutputAction::value_type value;

  remove (parameter);
  if (precondition (parameter)) {
    effect (parameter, &value);
    schedule ();
    finish (&value);
  }
  else {
    schedule ();
    finish (false);
  }
}

template <class OutputAction,
	  class Remove,
	  class Precondition,
	  class Effect,
	  class Schedule,
	  class Finish>
void
output_action (Remove remove,
	       Precondition precondition,
	       Effect effect,
	       Schedule schedule,
	       Finish finish)
{
  STATIC_ASSERT (is_output_action<OutputAction>::value);
  STATIC_ASSERT (OutputAction::parameter_mode == NO_PARAMETER);

  output_action_<OutputAction> (typename OutputAction::value_category (), remove, precondition, effect, schedule, finish);
}

template <class OutputAction,
	  class Remove,
	  class Precondition,
	  class Effect,
	  class Schedule,
	  class Finish>
void
output_action (typename OutputAction::parameter_type parameter,
	       Remove remove,
	       Precondition precondition,
	       Effect effect,
	       Schedule schedule,
	       Finish finish)
{
  STATIC_ASSERT (is_output_action<OutputAction>::value);
  STATIC_ASSERT (OutputAction::parameter_mode == PARAMETER || OutputAction::parameter_mode == AUTO_PARAMETER);

  output_action_<OutputAction> (typename OutputAction::value_category (), parameter, remove, precondition, effect, schedule, finish);
}

template <class InternalAction,
	  class Remove,
	  class Precondition,
	  class Effect,
	  class Schedule,
	  class Finish>
void
internal_action (Remove remove,
		 Precondition precondition,
		 Effect effect,
		 Schedule schedule,
		 Finish finish)
{
  STATIC_ASSERT (is_internal_action<InternalAction>::value);
  STATIC_ASSERT (InternalAction::parameter_mode == NO_PARAMETER);

  remove ();
  if (precondition ()) {
    effect ();
  }
  schedule ();
  finish ();
}

template <class InternalAction,
	  class Remove,
	  class Precondition,
	  class Effect,
	  class Schedule,
	  class Finish>
void
internal_action (typename InternalAction::parameter_type parameter,
		 Remove remove,
		 Precondition precondition,
		 Effect effect,
		 Schedule schedule,
		 Finish finish)
{
  STATIC_ASSERT (is_internal_action<InternalAction>::value);
  STATIC_ASSERT (InternalAction::parameter_mode == PARAMETER);

  remove (parameter);
  if (precondition (parameter)) {
    effect (parameter);
  }
  schedule ();
  finish ();
}

template <typename T>
inline aid_t
aid_cast (T v)
{
  STATIC_ASSERT (sizeof (T) == sizeof (aid_t));
  // Nasty C-style cast but it gets around static vs reinterpret casting issues.
  return (aid_t)v;
}

#endif /* __action_traits_hpp__ */
