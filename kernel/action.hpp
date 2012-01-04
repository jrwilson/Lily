#ifndef __action_descriptor_hpp__
#define __action_descriptor_hpp__

#include "action_traits.hpp"
#include "kstring.hpp"

class automaton;

// Partial action.
struct paction {
  // If size becomes a problem we can bit-pack the various modes.
  ::automaton* const automaton;
  kstring const name;
  kstring const description;
  action_type_t const type;
  const void* const action_entry_point;
  parameter_mode_t const parameter_mode;

  paction (::automaton* a,
	   const char* n,
	   const char* d,
	   action_type_t t,
	   const void* aep,
	   parameter_mode_t pm) :
    automaton (a),
    name (n),
    description (d),
    type (t),
    action_entry_point (aep),
    parameter_mode (pm)
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

// TODO:  Come up with a better hash function.
struct caction_hash {
  size_t
  operator() (const caction& c) const
  {
    return reinterpret_cast<uintptr_t> (c.action) ^ c.parameter;
  }
};

#endif /* __action_descriptor_hpp__ */
