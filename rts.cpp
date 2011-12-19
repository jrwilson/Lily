/*
  File
  ----
  rts.cpp
  
  Description
  -----------
  The run-time system (rts).

  Authors:
  Justin R. Wilson
*/

#include "rts.hpp"
#include <algorithm>
#include "automaton.hpp"
#include "buffer.hpp"
#include "scheduler.hpp"

automaton*
rts::create (descriptor_constants::privilege_t privilege,
	     frame_t frame)
{
  // Generate an id.
  aid_t aid = current_aid_;
  while (aid_map_.find (aid) != aid_map_.end ()) {
    aid = std::max (aid + 1, 0); // Handles overflow.
  }
  current_aid_ = std::max (aid + 1, 0);
  
  // Create the automaton and insert it into the map.
  automaton* a = new (system_alloc ()) automaton (aid, privilege, frame);
  aid_map_.insert (std::make_pair (aid, a));
  
  // Add to the scheduler.
  scheduler::add_automaton (a);
  
  return a;
}

void
rts::bind_ (automaton* output_automaton,
	    const void* output_action_entry_point,
	    parameter_mode_t output_parameter_mode,
	    aid_t output_parameter,
	    automaton* input_automaton,
	    const void* input_action_entry_point,
	    parameter_mode_t input_parameter_mode,
	    aid_t input_parameter,
	    size_t value_size)
{
  // Check the output action dynamically.
  kassert (output_automaton != 0);
  automaton::const_action_iterator output_pos = output_automaton->action_find (output_action_entry_point);
  kassert (output_pos != output_automaton->action_end () && output_pos->type == OUTPUT);
  
  // Check the input action dynamically.
  kassert (input_automaton != 0);
  automaton::const_action_iterator input_pos = input_automaton->action_find (input_action_entry_point);
  kassert (input_pos != input_automaton->action_end () && input_pos->type == INPUT);
  
  // TODO:  All of the bind checks.
  caction oa (output_automaton, OUTPUT, output_action_entry_point, output_parameter_mode, value_size, output_parameter);
  caction ia (input_automaton, INPUT, input_action_entry_point, input_parameter_mode, value_size, input_parameter);
  
  std::pair<bindings_type::iterator, bool> r = bindings_.insert (std::make_pair (oa, input_action_set_type ()));
  r.first->second.insert (ia);
}

bid_t
rts::buffer_create (size_t size,
		    automaton* a)
{
  // Generate an id.
  bid_t bid = current_bid_;
  while (bid_map_.find (bid) != bid_map_.end ()) {
    bid = std::max (bid + 1, 0); // Handles overflow.
  }
  current_bid_ = std::max (bid + 1, 0);
  
  // Create the buffer and insert it into the map.
  buffer* b = new (system_alloc ()) buffer (size);
  bid_map_.insert (std::make_pair (bid, b));

  // Start a reference count.
  reference_map_.insert (std::make_pair (reference_key (a, b), 1));

  return bid;
}

size_t
rts::buffer_size (bid_t bid,
		  automaton* a)
{
  bid_map_type::const_iterator bpos = bid_map_.find (bid);
  if (bpos != bid_map_.end ()) {
    buffer* b = bpos->second;
    if (reference_map_.find (reference_key (a, b)) != reference_map_.end ()) {
      // The buffer exists and the automaton has a reference.
      return b->size ();
    }
  }
  // The buffer does not exist or the automaton doesn't have a reference.
  return -1;
}

// TODO:  Reference count overflow.
int
rts::buffer_incref (bid_t bid,
		    automaton* a)
{
  bid_map_type::const_iterator bpos = bid_map_.find (bid);
  if (bpos != bid_map_.end ()) {
    buffer* b = bpos->second;
    reference_map_type::iterator rpos = reference_map_.find (reference_key (a, b));
    if (rpos != reference_map_.end ()) {
      // The buffer exists and the automaton has a reference.

      // Increment the count for the buffer.
      b->incref ();

      // Increment and return the count for the automaton.
      return ++rpos->second;
    }
  }
  // The buffer does not exist or the automaton doesn't have a reference.
  return -1;
}

int
rts::buffer_decref (bid_t bid,
		    automaton* a)
{
  bid_map_type::const_iterator bpos = bid_map_.find (bid);
  if (bpos != bid_map_.end ()) {
    buffer* b = bpos->second;
    reference_map_type::iterator rpos = reference_map_.find (reference_key (a, b));
    if (rpos != reference_map_.end ()) {
      // The buffer exists and the automaton has a reference.

      // Decrement the count for the buffer.
      if (b->decref () == 0) {
	// Purge the buffer.
	destroy (b, system_alloc ());
	bid_map_.erase (bpos);
      }

      // Decrement and return the count for the automaton.
      if (--rpos->second == 0) {
	reference_map_.erase (rpos);
	return 0;
      }

      return rpos->second;
    }
  }
  // The buffer does not exist or the automaton doesn't have a reference.
  return -1;
}

void*
rts::buffer_map (bid_t bid,
		 automaton* a)
{
  bid_map_type::const_iterator bpos = bid_map_.find (bid);
  if (bpos != bid_map_.end ()) {
    buffer* b = bpos->second;
    reference_map_type::iterator rpos = reference_map_.find (reference_key (a, b));
    if (rpos != reference_map_.end ()) {
      // The buffer exists and the automaton has a reference.
      return a->map (b);
    }
  }
  // The buffer does not exist or the automaton doesn't have a reference.
  return reinterpret_cast<void*> (-1);
}

aid_t rts::current_aid_ = 0;
rts::aid_map_type rts::aid_map_;

rts::bindings_type rts::bindings_;

bid_t rts::current_bid_ = 0;
rts::bid_map_type rts::bid_map_;

rts::reference_map_type rts::reference_map_;
