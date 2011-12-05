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

struct input_action_tag { };
struct output_action_tag { };
struct internal_action_tag { };

template <class Parameter,
	  class Message,
	  void (*Ptr) (Parameter, Message)>
struct input_action_traits {
  typedef input_action_tag action_category;
  typedef Parameter parameter_type;
  typedef Message message_type;
  static const size_t action_entry_point = reinterpret_cast<size_t> (Ptr);
};

template <class Parameter,
	  class Message,
	  void (*Ptr) (Parameter)>
struct output_action_traits {
  typedef output_action_tag action_category;
  typedef Parameter parameter_type;
  typedef Message message_type;
  static const size_t action_entry_point = reinterpret_cast<size_t> (Ptr);
};

template <class Parameter,
	  void (*Ptr) (Parameter)>
struct internal_action_traits {
  typedef internal_action_tag action_category;
  typedef Parameter parameter_type;
  static const size_t action_entry_point = reinterpret_cast<size_t> (Ptr);
};

// // TODO:  Get rid of reinterpret casts.
// template <class Tag,
// 	  class Parameter,
// 	  class Message,
// 	  void (*effect) (Parameter, Message&),
// 	  void (*schedule) (),
// 	  void (*finish) (bool, void*)>
// struct input_action {
//   typedef input_action_tag action_category;
//   typedef Parameter parameter_type;
//   typedef Message message_type;

//   static void
//   action (Parameter p,
// 	  Message& v)
//   {
//     effect (p, v);
//     schedule ();
//     finish (false, 0);
//   }

// };

template <class InputAction,
	  class Effect,
	  class Schedule,
	  class Finish>
void
input_action (typename InputAction::parameter_type p,
	      typename InputAction::message_type m,
	      Effect effect,
	      Schedule schedule,
	      Finish finish)
{
  effect (p, m);
  schedule ();
  finish (0);
}

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
  static typename OutputAction::message_type message;

  remove (OutputAction::action_entry_point, p);
  if (precondition (p)) {
    effect (p, message);
    schedule ();
    finish (&message);
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
