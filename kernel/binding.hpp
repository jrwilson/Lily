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

class binding {
private:
  bid_t bid_;
  caction const output_action_;
  caction const input_action_;
  automaton* const owner_;

  // Next binding id to allocate.
  static bid_t current_bid_;

  // Map from bid to binding.
  typedef unordered_map<bid_t, binding*> bid_to_binding_map_type;
  static bid_to_binding_map_type bid_to_binding_map_;

  static bid_t
  generate_bid (binding* b)
  {
    // Generate an id.
    bid_t bid = current_bid_;
    while (bid_to_binding_map_.find (bid) != bid_to_binding_map_.end ()) {
      bid = max (bid + 1, 0); // Handles overflow.
    }
    current_bid_ = max (bid + 1, 0);

    bid_to_binding_map_.insert (make_pair (bid, b));

    return bid;
  }

public:
  binding (const caction& oa,
	   const caction& ia,
	   automaton* o) :
    bid_ (generate_bid (this)),
    output_action_ (oa),
    input_action_ (ia),
    owner_ (o)
  { }

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

};

#endif /* __binding_hpp__ */
