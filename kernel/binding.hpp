#ifndef __binding_hpp__
#define __binding_hpp__

/*
  File
  ----
  binding.hpp
  
  Description
  -----------
  A binding represent a connection between the an output action in one automaton and an input action in another automaton.
  Additionally, bindings have an owner which is the automaton that created the binding in the first place.

  Authors
  -------
  Justin R. Wilson
*/

#include "lily/types.h"
#include "action.hpp"
#include "unordered_map.hpp"

class automaton;

struct binding {
  bid_t bid;
  caction const output_action;
  caction const input_action;
  shared_ptr<automaton> const owner;

  binding (bid_t b,
	   const caction& oa,
	   const caction& ia,
	   const shared_ptr<automaton>& o) :
    bid (b),
    output_action (oa),
    input_action (ia),
    owner (o)
  { }

  ~binding () {
    kassert (bid == -1);
  }

  bool
  enabled () const
  {
    return bid != -1;
  }
};

#endif /* __binding_hpp__ */
