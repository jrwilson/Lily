#ifndef __action_descriptor_hpp__
#define __action_descriptor_hpp__

#include <lily/types.h>
#include <lily/action.h>
#include "kstring.hpp"

#define LILY_LIMITS_MAX_VALUE_SIZE 512

class automaton;

enum action_type_t {
  INPUT = LILY_ACTION_INPUT,
  OUTPUT = LILY_ACTION_OUTPUT,
  INTERNAL = LILY_ACTION_INTERNAL,
};

enum parameter_mode_t {
  NO_PARAMETER = LILY_ACTION_NO_PARAMETER,
  PARAMETER = LILY_ACTION_PARAMETER,
  AUTO_PARAMETER = LILY_ACTION_AUTO_PARAMETER,
};

// Partial action.
struct paction {
  ::automaton* const automaton;
  action_type_t const type;
  parameter_mode_t const parameter_mode;
  size_t const id;
  const void* const action_entry_point;

  paction (::automaton* a,
	   action_type_t t,
	   parameter_mode_t pm,
	   size_t i,
	   const void* aep) :
    automaton (a),
    type (t),
    parameter_mode (pm),
    id (i),
    action_entry_point (aep)
  { }

private:
  paction (const paction&);
  paction& operator= (const paction&);
};

// Complete action.
struct caction {
  const paction* action;
  const void* parameter;

  caction () :
    action (0),
    parameter (0)
  { }

  caction (const paction* act,
	   const void* p) :
    action (act),
    parameter (p)
  { }
  
  inline bool
  operator== (const caction& other) const
  {
    return action == other.action && parameter == other.parameter;
  }
};

// TODO:  Come up with a better hash function.
struct caction_hash {
  size_t
  operator() (const caction& c) const
  {
    return reinterpret_cast<uintptr_t> (c.action) ^ reinterpret_cast<uintptr_t> (c.parameter);
  }
};

#endif /* __action_descriptor_hpp__ */
