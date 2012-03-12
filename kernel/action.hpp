#ifndef __action_descriptor_hpp__
#define __action_descriptor_hpp__

#include <lily/types.h>
#include <lily/action.h>
#include "kstring.hpp"

class automaton;
class buffer;

enum action_type_t {
  INPUT = LILY_ACTION_INPUT,
  OUTPUT = LILY_ACTION_OUTPUT,
  INTERNAL = LILY_ACTION_INTERNAL,
  SYSTEM_INPUT = LILY_ACTION_SYSTEM_INPUT,
};

enum parameter_mode_t {
  NO_PARAMETER = LILY_ACTION_NO_PARAMETER,
  PARAMETER = LILY_ACTION_PARAMETER,
  AUTO_PARAMETER = LILY_ACTION_AUTO_PARAMETER,
};

// Partial action.
struct paction {
  action_type_t const type;
  parameter_mode_t const parameter_mode;
  const void* const action_entry_point;
  ano_t const action_number;
  kstring const name;
  kstring const description;

  paction (action_type_t t,
	   parameter_mode_t pm,
	   const void* aep,
	   ano_t an,
	   const kstring& n,
	   const kstring& d) :
    type (t),
    parameter_mode (pm),
    action_entry_point (aep),
    action_number (an),
    name (n),
    description (d)
  { }

private:
  paction (const paction&);
  paction& operator= (const paction&);
};

// Complete action.
struct caction {
  ::automaton* automaton;
  const paction* action;
  int parameter;
  // For system inputs.
  buffer* buffer_a;
  buffer* buffer_b;

  caction () :
    automaton (0),
    action (0),
    parameter (0),
    buffer_a (0),
    buffer_b (0)
  { }

  caction (::automaton* a,
	   const paction* act,
	   int p) :
    automaton (a),
    action (act),
    parameter (p),
    buffer_a (0),
    buffer_b (0)
  { }

  caction (::automaton* au,
	   const paction* act,
	   int p,
	   buffer* a,
	   buffer* b) :
    automaton (au),
    action (act),
    parameter (p),
    buffer_a (a),
    buffer_b (b)
  { }
  
  inline bool
  operator== (const caction& other) const
  {
    return automaton == other.automaton && action == other.action && parameter == other.parameter;
  }
};

// TODO:  Come up with a better hash function.
struct caction_hash {
  size_t
  operator() (const caction& c) const
  {
    return reinterpret_cast<size_t> (c.automaton) ^ reinterpret_cast<size_t> (c.action) ^ c.parameter;
  }
};

#endif /* __action_descriptor_hpp__ */
