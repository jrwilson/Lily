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

struct action_tag { };
struct input_action_tag : public action_tag { };
struct output_action_tag : public action_tag { };
struct internal_action_tag : public action_tag { };

template <class Tag,
	  class Parameter,
	  class Value,
	  void (*effect) (Parameter, Value&),
	  void (*schedule) (),
	  void (*finish) (bool, void*)>
struct input_action {
  typedef input_action_tag action_category;
  typedef Parameter parameter_type;
  typedef Value value_type;

  static void
  action (Parameter p,
	  Value& v)
  {
    effect (p, v);
    schedule ();
    finish (false, 0);
  }

};

// TODO:  Check that sizeof (Parameter) == sizeof (void*) and sizeof (Value) <= LIMIT
template <class Tag,
	  class Parameter,
	  class Value,
	  void (*remove) (local_func, void*),
	  bool (*precondition) (Parameter),
	  void (*effect) (Parameter, Value&),
	  void (*schedule) (),
	  void (*finish) (bool, void*)>
struct output_action {
  typedef output_action_tag action_category;
  typedef Parameter parameter_type;
  typedef Value value_type;

  static void
  action (Parameter p)
  {
    static Value result;

    remove (reinterpret_cast<local_func> (&action), p);
    if (precondition (p)) {
      effect (p, result);
      schedule ();
      finish (true, &result);
    }
    else {
      schedule ();
      finish (false, 0);
    }
  }

};

// TODO:  Check that sizeof (Parameter) == sizeof (void*)
template <class Tag,
	  class Parameter,
	  void (*remove) (local_func, void*),
	  bool (*precondition) (Parameter),
	  void (*effect) (Parameter),
	  void (*schedule) (),
	  void (*finish) (bool, void*)>
struct internal_action {
  typedef internal_action_tag action_category;
  typedef Parameter parameter_type;

  static void
  action (Parameter p)
  {
    remove (reinterpret_cast<local_func> (&action), p);
    if (precondition (p)) {
      effect (p);
    }
    schedule ();
    finish (false, 0);
  }

};

#endif /* __action_macros_hpp__ */
