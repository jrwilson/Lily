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

struct binding {
  bid_t const bid;
  caction const output_action;
  caction const input_action;

  binding (bid_t b,
	   const caction& oa,
	   const caction& ia) :
    bid (b),
    output_action (oa),
    input_action (ia),
    enabled_ (true)
  { }

  ~binding () {
    kassert (!enabled ());
  }

  bool
  enabled () const;

  void
  disable () {
    enabled_ = false;
  }

private:
  bool enabled_;
};

#endif /* __binding_hpp__ */
