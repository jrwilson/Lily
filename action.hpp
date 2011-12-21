#ifndef __action_descriptor_hpp__
#define __action_descriptor_hpp__

#include "action_traits.hpp"

class automaton;

// Partial action.
struct paction {
  action_type_t type;
  const void* action_entry_point;
  parameter_mode_t parameter_mode;
  buffer_value_mode_t buffer_value_mode;
  copy_value_mode_t copy_value_mode;
  size_t copy_value_size;

  paction () { }

  paction (action_type_t t,
	   const void* aep,
	   parameter_mode_t pm,
	   buffer_value_mode_t bvm,
	   copy_value_mode_t cvm,
	   size_t vs) :
    type (t),
    action_entry_point (aep),
    parameter_mode (pm),
    buffer_value_mode (bvm),
    copy_value_mode (cvm),
    copy_value_size (vs)
  { }
};

// Complete action.
struct caction : public paction {
  ::automaton* automaton;
  aid_t parameter;

  caction () { }
  
  caction (::automaton* a,
	   const paction& act,
	   aid_t p) :
    paction (act),
    automaton (a),
    parameter (p)
  { }

  caction (::automaton* a,
	   action_type_t t,
	   const void* aep,
	   parameter_mode_t pm,
	   buffer_value_mode_t bvm,
	   copy_value_mode_t cvm,
	   size_t vs,
	   aid_t p) :
    paction (t, aep, pm, bvm, cvm, vs),
    automaton (a),
    parameter (p)
  { }
  
  inline bool
  operator== (const caction& other) const
  {
    return automaton == other.automaton && action_entry_point == other.action_entry_point && parameter == other.parameter;
  }
};

#endif /* __action_descriptor_hpp__ */
