#include "binding.hpp"
#include "automaton.hpp"

bid_t binding::current_bid_ = 0;
binding::bid_to_binding_map_type binding::bid_to_binding_map_;

void
binding::incref ()
{
  output_action_.automaton->incref ();
  input_action_.automaton->incref ();
  owner_->incref ();
  // TODO:  This needs to be atomic.
  ++refcount_;
}

void
binding::decref ()
{
  output_action_.automaton->decref ();
  input_action_.automaton->decref ();
  owner_->decref ();
  // TODO:  This needs to be atomic.
  --refcount_;
  if (refcount_ == 0) {
    delete this;
  }
}
