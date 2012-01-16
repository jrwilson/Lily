#ifndef __action_descriptor_hpp__
#define __action_descriptor_hpp__

#include "kstring.hpp"

class automaton;

#define M_NO_COMPARE 0
#define M_EQUAL 1

#define M_INPUT 0
#define M_OUTPUT 1
#define M_INTERNAL 2

enum action_type_t {
  INPUT = M_INPUT,
  OUTPUT = M_OUTPUT,
  INTERNAL = M_INTERNAL,
};

enum compare_method_t {
  NO_COMPARE = M_NO_COMPARE,
  EQUAL = M_EQUAL,
};

#define M_NO_PARAMETER 0
#define M_PARAMETER 1
#define M_AUTO_PARAMETER 2

enum parameter_mode_t {
  NO_PARAMETER = M_NO_PARAMETER,
  PARAMETER = M_PARAMETER,
  AUTO_PARAMETER = M_AUTO_PARAMETER,
};

// Partial action.
struct paction {
  ::automaton* const automaton;
  kstring const name;
  kstring const description;
  compare_method_t const compare_method;
  action_type_t const type;
  const void* const action_entry_point;
  parameter_mode_t const parameter_mode;

  paction (::automaton* a,
	   const char* n,
	   const char* d,
	   compare_method_t cm,
	   action_type_t t,
	   const void* aep,
	   parameter_mode_t pm) :
    automaton (a),
    name (n),
    description (d),
    compare_method (cm),
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
