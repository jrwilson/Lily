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
  buffer* b = new (system_alloc ()) buffer (bid, size);
  bid_map_.insert (std::make_pair (bid, b));

  // Start a reference count.
  reference_map_.insert (std::make_pair (reference_key (a, b), 1));
  ++b->reference_count;

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
      return b->size;
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
    if (reference_map_.find (reference_key (a, b)) != reference_map_.end ()) {
      // The buffer exists and the automaton has a reference.

      // Changing the reference count always closes a buffer.
      b->status = buffer::CLOSED;

      // Increment the reference count for the implied set.
      for (buffer::implied_set_type::const_iterator pos = b->implied_set.begin ();
	   pos != b->implied_set.end ();
	   ++pos) {
	++reference_map_.find (reference_key (a, *pos))->second;
	++(*pos)->reference_count;
      }

      // Return the reference count for this buffer.     
      return reference_map_.find (reference_key (a, b))->second;
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
    if (reference_map_.find (reference_key (a, b)) != reference_map_.end ()) {
      // The buffer exists and the automaton has a reference.

      // Changing the reference count always closes a buffer.
      b->status = buffer::CLOSED;

      // Increment the reference count for the implied set.
      for (buffer::implied_set_type::const_iterator pos = b->implied_set.begin ();
	   pos != b->implied_set.end ();
	   ++pos) {
	// Decrement the reference count for the automaton and clean up.
	reference_map_type::iterator r = reference_map_.find (reference_key (a, *pos));
	if (--r->second == 0) {
	  reference_map_.erase (r);
	}

	// Decrement the global reference count and clean up if the buffer is not the one we are iterating over.
	if (--(*pos)->reference_count == 0 && *pos != b) {
	  bid_map_.erase ((*pos)->bid);
	  destroy ((*pos), system_alloc ());
	}
      }

      if (b->reference_count != 0) {
	// Return the reference count for this buffer.     
	return b->reference_count;
      }
      else {
	bid_map_.erase (bpos);
	destroy (b, system_alloc ());
	return 0;
      }
    }
  }
  // The buffer does not exist or the automaton doesn't have a reference.
  return -1;
}

int
rts::buffer_addchild (bid_t parent,
		      bid_t child,
		      automaton* a)
{
  bid_map_type::const_iterator parent_pos = bid_map_.find (parent);
  bid_map_type::const_iterator child_pos = bid_map_.find (child);
  if (parent_pos != bid_map_.end () && child_pos != bid_map_.end ()) {
    // Buffers exist.
    buffer* parent_buffer = parent_pos->second;
    buffer* child_buffer = child_pos->second;

    // The parent buffer is open meaning the automaton is the only one with a reference.
    // The automaton has a reference to both the parent and child.
    if (parent_buffer->status == buffer::OPEN &&
	reference_map_.find (reference_key (a, parent_buffer)) != reference_map_.end () &&
	reference_map_.find (reference_key (a, child_buffer)) != reference_map_.end ()) {
      // The child buffer is closed if it was open.
      child_buffer->status = buffer::CLOSED;

      // Iterate over the buffers implied by the child.
      for (buffer::implied_set_type::iterator pos = child_buffer->implied_set.begin ();
	   pos != child_buffer->implied_set.end ();
	   ++pos) {
	// Insert each one.
	std::pair<buffer::implied_set_type::iterator, bool> r = parent_buffer->implied_set.insert (*pos);
	if (r.second) {
	  // This is a new child.  Transfer a reference to the child.
	  std::pair<reference_map_type::iterator, bool> s = reference_map_.insert (std::make_pair (reference_key (a, *pos), 0));
	  ++s.first->second;
	  ++(*pos)->reference_count;
	}
      }
      
      return 0;
    }
  }
  return -1;
}

aid_t rts::current_aid_ = 0;
rts::aid_map_type rts::aid_map_;

rts::bindings_type rts::bindings_;

bid_t rts::current_bid_ = 0;
rts::bid_map_type rts::bid_map_;

rts::reference_map_type rts::reference_map_;
