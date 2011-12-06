#ifndef __action_macros_hpp__
#define __action_macros_hpp__

/*
  File
  ----
  action_macros.hpp
  
  Description
  -----------
  Macros for declaring actions.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"

struct null_type { };

// Parameters.
struct no_parameter_tag { };
struct parameter_tag { };
struct auto_parameter_tag { };

struct no_parameter {
  typedef no_parameter_tag parameter_category;
  typedef null_type parameter_type;
};

template <class Parameter>
struct parameter {
  typedef parameter_tag parameter_category;
  typedef Parameter parameter_type;
};

struct auto_parameter {
  typedef auto_parameter_tag parameter_category;
  typedef aid_t parameter_type;
};

// Values.
struct no_value_tag { };
struct value_tag { };

struct no_value {
  typedef no_value_tag value_category;
  typedef null_type value_type;
};

template <class Value>
struct value {
  typedef value_tag value_category;
  typedef Value value_type;
};

// Actions.
struct input_action_tag { };
struct output_action_tag { };
struct internal_action_tag { };

struct input_action {
  typedef input_action_tag action_category;
};

struct output_action {
  typedef output_action_tag action_category;
};

struct internal_action {
  typedef internal_action_tag action_category;
};

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

template <class InputAction,
	  class Effect,
	  class Schedule,
	  class Finish>
void
input_action (typename InputAction::parameter_type p,
	      Effect effect,
	      Schedule schedule,
	      Finish finish)
{
  effect (p);
  schedule ();
  finish (0);
}

template <class InputAction,
	  class Effect,
	  class Schedule,
	  class Finish>
void
input_action (typename InputAction::value_type m,
	      Effect effect,
	      Schedule schedule,
	      Finish finish)
{
  effect (m);
  schedule ();
  finish (0);
}

template <class InputAction,
	  class Effect,
	  class Schedule,
	  class Finish>
void
input_action (typename InputAction::parameter_type p,
	      typename InputAction::value_type m,
	      Effect effect,
	      Schedule schedule,
	      Finish finish)
{
  effect (p, m);
  schedule ();
  finish (0);
}

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

template <class OutputAction,
	  class Remove,
	  class Precondition,
	  class Effect,
	  class Schedule,
	  class Finish>
void
output_action (typename OutputAction::parameter_type p,
	       Remove remove,
	       Precondition precondition,
	       Effect effect,
	       Schedule schedule,
	       Finish finish)
{
  static typename OutputAction::value_type value;

  remove (OutputAction::action_entry_point, p);
  if (precondition (p)) {
    effect (p, value);
    schedule ();
    finish (&value);
  }
  else {
    schedule ();
    finish (0);
  }
}

template <class InternalAction,
	  class Remove,
	  class Precondition,
	  class Effect,
	  class Schedule,
	  class Finish>
void
internal_action (typename InternalAction::parameter_type p,
		 Remove remove,
		 Precondition precondition,
		 Effect effect,
		 Schedule schedule,
		 Finish finish)
{
  remove (InternalAction::action_entry_point, p);
  if (precondition (p)) {
    effect (p);
  }
  schedule ();
  finish (0);
}

#endif /* __action_macros_hpp__ */
