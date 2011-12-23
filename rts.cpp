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
rts::create (bool privcall,
	     vm::page_privilege_t map_privilege,
	     vm::page_privilege_t kernel_privilege)
{
  // Allocate a frame.
  frame_t frame = frame_manager::alloc ();
  // Map the page directory.
  vm::map (vm::get_stub1 (), frame, vm::USER, vm::WRITABLE);
  vm::page_directory* pd = reinterpret_cast<vm::page_directory*> (vm::get_stub1 ());
  // Initialize the page directory.
  // If the second argument is vm::USER, the automaton can access the paging area, i.e., manipulate virtual memory.
  // If the second argument is vm::SUPERVISOR, the automaton cannot.
  // This is useful for the system automaton because it maps in frames to create page directories by calling this function.
  // However, it is not necessary as it can be done using buffers.
  new (pd) vm::page_directory (frame, map_privilege);
  // Initialize the page directory with a copy of the kernel.
  // If the third argument is vm::USER, the automaton gains access to kernel data.
  // If the third argument is vm::SUPERVISOR, the automaton does not.
  vm::copy_page_directory (pd, vm::get_kernel_page_directory (), kernel_privilege);
  // Unmap.
  vm::unmap (vm::get_stub1 ());
  // Drop the reference count from allocation.
  size_t count = frame_manager::decref (frame);
  kassert (count == 1);

  // Generate an id.
  aid_t aid = current_aid_;
  while (aid_map_.find (aid) != aid_map_.end ()) {
    aid = std::max (aid + 1, 0); // Handles overflow.
  }
  current_aid_ = std::max (aid + 1, 0);
 
  // Create the automaton and insert it into the map.
  automaton* a = new (system_alloc ()) automaton (aid, frame, privcall);
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
	    buffer_value_mode_t buffer_value_mode,
	    copy_value_mode_t copy_value_mode,
	    size_t copy_value_size)
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
  caction oa (output_automaton, OUTPUT, output_action_entry_point, output_parameter_mode, buffer_value_mode, copy_value_mode, copy_value_size, output_parameter);
  caction ia (input_automaton, INPUT, input_action_entry_point, input_parameter_mode, buffer_value_mode, copy_value_mode, copy_value_size, input_parameter);
  
  std::pair<bindings_type::iterator, bool> r = bindings_.insert (std::make_pair (oa, input_action_set_type ()));
  r.first->second.insert (ia);
}

aid_t rts::current_aid_ = 0;
rts::aid_map_type rts::aid_map_;

rts::bindings_type rts::bindings_;
