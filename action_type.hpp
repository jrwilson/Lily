#ifndef __action_type_hpp__
#define __action_type_hpp__

/*
  File
  ----
  action_type.hpp
  
  Description
  -----------
  Enum for action types.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"

enum action_type_t {
  NO_ACTION = 0,
  INPUT,
  OUTPUT,
  INTERNAL
};

class automaton;

struct action {
  ::automaton* automaton;
  void* action_entry_point;
  parameter_t parameter;

  action () :
    automaton (0),
    action_entry_point (0),
    parameter (0)
  { }

  action (::automaton* a,
	  void* aep,
	  parameter_t p) :
    automaton (a),
    action_entry_point (aep),
    parameter (p)
  { }
  
  bool
  operator== (const action& other) const
  {
    return automaton == other.automaton && action_entry_point == other.action_entry_point && parameter == other.parameter;
  }
};

struct input_action : public action {
  input_action (::automaton* a,
		void* aep,
		parameter_t p) :
    action (a, aep, p)
  { }
};

struct local_action : public action {
  local_action ()
  { }

  local_action (::automaton* a,
		void* aep,
		parameter_t p) :
    action (a, aep, p)
  { }
};

struct output_action : public local_action {
  output_action (::automaton* a,
		 void* aep,
		 parameter_t p) :
    local_action (a, aep, p)
  { }
};

struct internal_action : public local_action {
  internal_action (::automaton* a,
		   void* aep,
		   parameter_t p) :
    local_action (a, aep, p)
  { }
};

#endif /* __action_type_hpp__ */
