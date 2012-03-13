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

class binding {
private:
  bid_t bid_;
  caction const output_action_;
  caction const input_action_;
  shared_ptr<automaton> const owner_;
  bool enabled_;

public:
  binding (bid_t bid,
	   const caction& oa,
	   const caction& ia,
	   const shared_ptr<automaton>& o) :
    bid_ (bid),
    output_action_ (oa),
    input_action_ (ia),
    owner_ (o),
    enabled_ (true)
  { }

  ~binding () {
    // TODO:  Return the bid.  This also needs to be atomic.
    kassert (0);
  }

  bid_t
  bid () const
  {
    return bid_;
  }

  const caction&
  output_action () const
  {
    return output_action_;
  }

  const caction&
  input_action () const
  {
    return input_action_;
  }
  
  const shared_ptr<automaton>&
  owner () const
  {
    return owner_;
  }

  bool
  enabled () const
  {
    return enabled_;
  }

  void
  disable ()
  {
    enabled_ = false;
  }
};

#endif /* __binding_hpp__ */
