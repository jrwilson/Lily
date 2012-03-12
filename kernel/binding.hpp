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
  // Next binding id to allocate.
  static bid_t current_bid_;

  // Map from bid to binding.
  typedef unordered_map<bid_t, binding*> bid_to_binding_map_type;
  static bid_to_binding_map_type bid_to_binding_map_;

  static bid_t
  generate_bid (binding* b)
  {
    // TODO:  This needs to be atomic.

    // Generate an id.
    bid_t bid = current_bid_;
    while (bid_to_binding_map_.find (bid) != bid_to_binding_map_.end ()) {
      bid = max (bid + 1, 0); // Handles overflow.
    }
    current_bid_ = max (bid + 1, 0);

    bid_to_binding_map_.insert (make_pair (bid, b));

    return bid;
  }

  bid_t bid_;
  caction const output_action_;
  caction const input_action_;
  automaton* const owner_;
  size_t refcount_;
  bool enabled_;

  ~binding () {
    // TODO:  Return the bid.  This also needs to be atomic.
    kassert (0);
  }

public:
  binding (const caction& oa,
	   const caction& ia,
	   automaton* o) :
    bid_ (generate_bid (this)),
    output_action_ (oa),
    input_action_ (ia),
    owner_ (o),
    refcount_ (1),
    enabled_ (true)
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
  
  automaton*
  owner () const
  {
    return owner_;
  }

  void
  incref ();

  void
  decref ();

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
