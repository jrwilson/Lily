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
  
  // Create the automaton and insert it into the map.
  buffer* b = new (system_alloc ()) buffer (size, a);
  bid_map_.insert (std::make_pair (bid, b));
  
  return bid;
}

size_t
rts::buffer_size (bid_t bid,
		  automaton* a)
{
  bid_map_type::const_iterator pos = bid_map_.find (bid);
  if (pos != bid_map_.end ()) {
    return pos->second->size (a);
  }
  else {
    // The buffer does not exist.
    return -1;
  }
}

int
rts::buffer_incref (bid_t bid,
		    automaton* a)
{
  bid_map_type::const_iterator pos = bid_map_.find (bid);
  if (pos != bid_map_.end ()) {
    return pos->second->incref (a);
  }
  else {
    // The buffer does not exist.
    return -1;
  }  
}

int
rts::buffer_decref (bid_t bid,
		    automaton* a)
{
  bid_map_type::const_iterator pos = bid_map_.find (bid);
  if (pos != bid_map_.end ()) {
    int retval = pos->second->decref (a);
    // Remove the buffer if there are not more references.
    if (pos->second->empty ()) {
      destroy (pos->second, system_alloc ());
      bid_map_.erase (pos);
    }
    return retval;
  }
  else {
    // The buffer does not exist.
    return -1;
  }  
}

aid_t rts::current_aid_ = 0;
rts::aid_map_type rts::aid_map_;

rts::bindings_type rts::bindings_;

bid_t rts::current_bid_ = 0;
rts::bid_map_type rts::bid_map_;
