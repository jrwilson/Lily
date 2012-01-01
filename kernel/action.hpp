#ifndef __action_descriptor_hpp__
#define __action_descriptor_hpp__

#include "action_traits.hpp"

class automaton;

// TODO:  Can we make these smaller?

// Partial action.
struct paction {
  ::automaton* const automaton;
  action_type_t const type;
  const void* const action_entry_point;
  parameter_mode_t const parameter_mode;
  buffer_value_mode_t const buffer_value_mode;
  copy_value_mode_t const copy_value_mode;
  size_t const copy_value_size;

  paction (::automaton* a,
	   action_type_t t,
	   const void* aep,
	   parameter_mode_t pm,
	   buffer_value_mode_t bvm,
	   copy_value_mode_t cvm,
	   size_t vs) :
    automaton (a),
    type (t),
    action_entry_point (aep),
    parameter_mode (pm),
    buffer_value_mode (bvm),
    copy_value_mode (cvm),
    copy_value_size (vs)
  { }

private:
  paction (const paction&);
  paction& operator= (const paction&);
};

// Complete action.
struct caction {
  const paction* action;
  aid_t parameter;

  caction () :
    action (0),
    parameter (0)
  { }

  caction (const paction* act,
	   aid_t p) :
    action (act),
    parameter (p)
  { }
  
  inline bool
  operator== (const caction& other) const
  {
    return action == other.action && parameter == other.parameter;
  }
};

#endif /* __action_descriptor_hpp__ */
