#ifndef __action_descriptor_hpp__
#define __action_descriptor_hpp__

#include <lily/types.h>
#include <lily/action.h>
#include "buffer.hpp"

class automaton;

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
  ::automaton* const automaton;
  action_type_t const type;
  parameter_mode_t const parameter_mode;
  const void* const action_entry_point;
  ano_t const action_number;

  paction (::automaton* a,
	   action_type_t t,
	   parameter_mode_t pm,
	   const void* aep,
	   ano_t an) :
    automaton (a),
    type (t),
    parameter_mode (pm),
    action_entry_point (aep),
    action_number (an)
  { }

private:
  paction (const paction&);
  paction& operator= (const paction&);
};

// Complete action.
struct caction {
  aid_t aid;
  action_type_t type;
  parameter_mode_t parameter_mode;
  const void* action_entry_point;
  ano_t action_number;
  int parameter;
  // For system inputs.
  buffer* buffer_a;
  buffer* buffer_b;

  caction () :
    aid (-1),
    action_number (LILY_ACTION_NO_ACTION),
    buffer_a (0),
    buffer_b (0)
  { }

  caction (aid_t aid,
	   const paction* act,
	   int p) :
    aid (aid),
    type (act->type),
    parameter_mode (act->parameter_mode),
    action_entry_point (act->action_entry_point),
    action_number (act->action_number),
    parameter (p),
    buffer_a (0),
    buffer_b (0)
  { }

  caction (aid_t aid,
	   const paction* act,
	   int p,
	   buffer* a,
	   buffer* b) :
    aid (aid),
    type (act->type),
    parameter_mode (act->parameter_mode),
    action_entry_point (act->action_entry_point),
    action_number (act->action_number),
    parameter (p),
    buffer_a (a),
    buffer_b (b)
  { }
  
  inline bool
  operator== (const caction& other) const
  {
    return aid == other.aid && action_number == other.action_number && parameter == other.parameter;
  }
};

// TODO:  Come up with a better hash function.
struct caction_hash {
  size_t
  operator() (const caction& c) const
  {
    return c.aid ^ c.action_number ^ c.parameter;
  }
};

#endif /* __action_descriptor_hpp__ */
