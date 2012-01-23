#include "rts.hpp"
#include "kassert.hpp"
#include "vm.hpp"
#include "automaton.hpp"
#include "scheduler.hpp"
#include "elf.hpp"
#include "bitset.hpp"
#include "mapped_area.hpp"
#include "lily/syscall.h"

namespace rts {
  
  // Bitset marking which frames are used for memory-mapped I/O.
  // I assume the region from 0x0 to 0x00100000 is used for memory-mapped I/O.
  // Need one bit for each frame.
  static bitset<ONE_MEGABYTE / PAGE_SIZE> mmapped_frames_;

  // For creating automata and allocating them an aid.
  static aid_t current_aid_;
  typedef unordered_map<aid_t, automaton*> aid_map_type;
  static aid_map_type aid_map_;

  struct map_op {
    logical_address_t logical_address;
    frame_t frame;
    vm::map_mode_t map_mode;

    map_op (logical_address_t la,
  	    frame_t f,
  	    vm::map_mode_t m) :
      logical_address (la),
      frame (f),
      map_mode (m)
    { }
  };

  // A list of frames used when creating automata.
  typedef unordered_map<logical_address_t, pair<frame_t, vm::map_mode_t> > frame_map_type;
  static frame_map_type frame_map_;

  typedef vector<pair<logical_address_t, logical_address_t> > clear_list_type;
  static clear_list_type clear_list_;

  static automaton*
  create_automaton (bool privileged,
		    logical_address_t text_begin,
  		    logical_address_t text_end)
  {
    // Create the automaton.

    // Allocate a frame.
    frame_t frame = frame_manager::alloc ();
    // Map the page directory.
    vm::map (vm::get_stub1 (), frame, vm::USER, vm::MAP_READ_WRITE, false);
    vm::page_directory* pd = reinterpret_cast<vm::page_directory*> (vm::get_stub1 ());
    // Initialize the page directory with a copy of the kernel.
    // Since the second argument is vm::SUPERVISOR, the automaton cannot access the paging area, i.e., manipulate virtual memory.
    // If the third argument is vm::USER, the automaton gains access to kernel data.
    // If the third argument is vm::SUPERVISOR, the automaton does not gain access to kernel data.
    new (pd) vm::page_directory (frame, vm::SUPERVISOR, vm::SUPERVISOR);
    // Unmap.
    vm::unmap (vm::get_stub1 ());
    // Drop the reference count from allocation.
    size_t count = frame_manager::decref (frame);
    kassert (count == 1);

    // Generate an id.
    aid_t aid = current_aid_;
    while (aid_map_.find (aid) != aid_map_.end ()) {
      aid = max (aid + 1, 0); // Handles overflow.
    }
    current_aid_ = max (aid + 1, 0);
    
    // Create the automaton and insert it into the map.
    automaton* a = new automaton (aid, privileged, frame);
    aid_map_.insert (make_pair (aid, a));
    
    // Add to the scheduler.
    scheduler::add_automaton (a);

    // Parse the file.
    elf::parser hp;
    if (!hp.parse (a, text_begin, text_end)) {
      // BUG:  Couldn't parse the program.
      kassert (0);
    }

    // Add the actions to the automaton.
    for (elf::parser::action_iterator pos = hp.action_begin (); pos != hp.action_end (); ++pos) {
      if (!a->add_action (*pos)) {
    	// BUG:  The action conflicts with an existing action.  Be sure to delete the rest of the pointers.
    	kassert (0);
      }
    }

    // Create the memory map for the automaton.
    frame_map_.clear ();
    clear_list_.clear ();
    for (elf::parser::program_header_iterator e = hp.program_header_begin (); e != hp.program_header_end (); ++e) {
      vm::map_mode_t map_mode = ((e->permissions & elf::WRITE) != 0) ? vm::MAP_COPY_ON_WRITE : vm::MAP_READ_ONLY;
      
      size_t s;
      // Initialized data.
      for (s = 0; s < e->file_size; s += PAGE_SIZE) {
    	pair<frame_map_type::iterator, bool> r = frame_map_.insert (make_pair (align_down (e->virtual_address + s, PAGE_SIZE), make_pair (vm::logical_address_to_frame (text_begin + e->offset + s), map_mode)));
	if (!r.second && map_mode == vm::MAP_COPY_ON_WRITE) {
	  // Already existed and needs to be copy-on-write.
	  r.first->second.second = vm::MAP_COPY_ON_WRITE;
	}
      }
      
      // Clear the tiny region between the end of initialized data and the first unitialized page.
      if (e->file_size < e->memory_size) {
    	logical_address_t begin = e->virtual_address + e->file_size;
    	logical_address_t end = e->virtual_address + e->memory_size;
    	clear_list_.push_back (make_pair (begin, end));
      }
      
      // Uninitialized data.
      for (; s < e->memory_size; s += PAGE_SIZE) {
   	pair<frame_map_type::iterator, bool> r = frame_map_.insert (make_pair (align_down (e->virtual_address + s, PAGE_SIZE), make_pair (vm::zero_frame (), map_mode)));
	if (!r.second && map_mode == vm::MAP_COPY_ON_WRITE) {
	  // Already existed and needs to be copy-on-write.
	  r.first->second.second = vm::MAP_COPY_ON_WRITE;
	}
      }
      
      if (!a->vm_area_is_free (e->virtual_address, e->virtual_address + e->memory_size)) {
    	// BUG:  The area conflicts with the existing memory map.
    	kassert (0);
      }

      a->insert_vm_area (new vm_area_base (e->virtual_address, e->virtual_address + e->memory_size));
    }

    if (!a->insert_heap_and_stack ()) {
      // BUG
      kassert (0);
    }

    // Switch to the automaton.
    physical_address_t old = vm::switch_to_directory (a->page_directory_physical_address ());
    
    // We can only use data in the kernel, i.e., we can't use automaton_text or hp.
    
    // Map all the frames.
    for (frame_map_type::const_iterator pos = frame_map_.begin ();
    	 pos != frame_map_.end ();
    	 ++pos) {
      vm::map (pos->first, pos->second.first, vm::USER, pos->second.second, false);
    }

    // Clear.
    for (clear_list_type::const_iterator pos = clear_list_.begin ();
    	 pos != clear_list_.end ();
    	 ++pos) {
      memset (reinterpret_cast<void*> (pos->first), 0, pos->second - pos->first);
    }

    // Map the heap and stack while using the automaton's page directory.
    a->map_heap_and_stack ();

    // Switch back.
    vm::switch_to_directory (old);

    return a;
  }

  // static bool
  // bind_actions (automaton* output_automaton,
  // 		const kstring& output_action_name,
  // 		aid_t output_parameter,
  // 		automaton* input_automaton,
  // 		const kstring& input_action_name,
  // 		aid_t input_parameter,
  // 		automaton* owner)
  // {
  //   kassert (output_automaton != 0);
  //   kassert (input_automaton != 0);
  //   kassert (owner != 0);

  //   if (output_automaton == input_automaton) {
  //     // The output and input automata must be different.
  //     return false;
  //   }

  //   // Check the output action dynamically.
  //   const paction* output_action = output_automaton->find_action (output_action_name);
  //   if (output_action == 0 ||
  // 	output_action->type != OUTPUT) {
  //     // Output action does not exist or has the wrong type.
  //     return false;
  //   }

  //   // Check the input action dynamically.
  //   const paction* input_action = input_automaton->find_action (input_action_name);
  //   if (input_action == 0 ||
  // 	input_action->type != INPUT) {
  //     // Input action does not exist or has the wrong type.
  //     return false;
  //   }

  //   // Check the descriptions.
  //   if (output_action->compare_method != input_action->compare_method) {
  //     return false;
  //   }

  //   switch (output_action->compare_method) {
  //   case NO_COMPARE:
  //     // Okay.  I won't.
  //     break;
  //   case EQUAL:
  //     // Simple string equality.
  //     if (output_action->description != input_action->description) {
  // 	return false;
  //     }
  //     break;
  //   }

  //   // Adust the parameters.
  //   switch (output_action->parameter_mode) {
  //   case NO_PARAMETER:
  //     output_parameter = 0;
  //     break;
  //   case PARAMETER:
  //     break;
  //   case AUTO_PARAMETER:
  //     output_parameter = input_automaton->aid ();
  //     break;
  //   }

  //   switch (input_action->parameter_mode) {
  //   case NO_PARAMETER:
  //     input_parameter = 0;
  //     break;
  //   case PARAMETER:
  //     break;
  //   case AUTO_PARAMETER:
  //     input_parameter = output_automaton->aid ();
  //     break;
  //   }

  //   // Form complete actions.
  //   caction oa (output_action, output_parameter);
  //   caction ia (input_action, input_parameter);

  //   // Check that the input action is not bound.
  //   // (Also checks that the binding does not exist.)
  //   if (input_automaton->is_input_bound (ia)) {
  //     return false;
  //   }

  //   // Check that the output action is not bound to an action in the input automaton.
  //   if (output_automaton->is_output_bound_to_automaton (oa, input_automaton)) {
  //     return false;
  //   }

  //   // Bind.
  //   output_automaton->bind_output (oa, ia, owner);
  //   input_automaton->bind_input (oa, ia, owner);
  //   owner->bind (oa, ia);

  //   return true;
  // }

  void
  create_init_automaton (frame_t automaton_frame_begin,
			 size_t automaton_size,
			 frame_t data_frame_begin,
			 size_t data_size)
  {
    // Map the text of the initial automaton right below the paging area to avoid the heap.
    const logical_address_t text_begin = PAGING_AREA - align_up (automaton_size, PAGE_SIZE);
    const logical_address_t text_end = text_begin + automaton_size;
    const size_t automaton_frame_end = automaton_frame_begin + align_up (automaton_size, PAGE_SIZE) / PAGE_SIZE;

    logical_address_t address = text_begin;
    for (size_t frame = automaton_frame_begin; frame != automaton_frame_end; ++frame, address += PAGE_SIZE) {
      vm::map (address, frame, vm::USER, vm::MAP_READ_ONLY, false);
      // Drop the reference count back to 1.
      size_t count = frame_manager::decref (frame);
      kassert (count == 1);
    }

    automaton* child = create_automaton (true, text_begin, text_end);

    // Unmap the program text.
    for (address = text_begin; address < text_end; address += PAGE_SIZE) {
      vm::unmap (address);
    }

    // Create a buffer to contain the initializationd data.
    bid_t data_bid = child->buffer_create (0);
    kassert (data_bid == 0);
    buffer* data_buffer = child->lookup_buffer (data_bid);
    
    // Add the frames manually.
    const size_t data_frame_end = data_frame_begin + align_up (data_size, PAGE_SIZE) / PAGE_SIZE;
    for (frame_t frame = data_frame_begin; frame != data_frame_end; ++frame) {
      data_buffer->append_frame (frame);
      // Drop the reference count.
      size_t count = frame_manager::decref (frame);
      kassert (count == 1);
    }

    // Schedule the init action.
    const paction* action = child->find_action (kstring ("init"));

    if (action == 0) {
      kout << "The initial automaton does not contain an init action.  Halting." << endl;
      halt ();
    }
    
    if (action->type != INTERNAL) {
      kout << "The init action of the initial automaton is not an internal action.  Halting." << endl;
      halt ();
    }

    if (action->parameter_mode != PARAMETER) {
      kout << "The init action of the initial automaton must take a parameter.  Halting." << endl;
      halt ();
    }

    scheduler::schedule (caction (action, reinterpret_cast<const void*> (data_size)));

    // Start the scheduler.  Doesn't return.
    scheduler::finish (0, 0, -1, 0);
  }

  // The automaton has finished the current action.
  // The parameters are interrupted as follows:
  //
  //   action_entry_point and parameter can be used to schedule another action.
  //   These are ignored if action_entry_point is 0.
  //
  //   value and value_size specify the value produced by an output action.
  //   These are ignored for input and internal actions.
  //   For an output action, they are ignored if value is 0.
  //   The range [value, value + value_size) must appear in the addres space of the automaton.
  //   value_size must be less than or equal to LILY_LIMITS_MAX_VALUE_SIZE.
  //   
  //   buffer and buffer_size specify the buffer produced by an output action.
  //   These are ignored for input and internal actions.
  //   For an output action, they are ignored if buffer refers to a buffer that does not exist.
  //   -1 always refers to a buffer that doesn't exist.
  //   buffer_size must be less than or equal to the physical size of the buffer.
  //   
  //   If either a value or a buffer is specified, the output is said to have "fired."
  //   An output can "fire" without producing a value or buffer by specifying a value that is not zero with a value_size that is zero.
  void
  finish (const caction& current,
	  const void* action_entry_point,
	  const void* parameter,
	  const void* value,
	  size_t value_size,
	  bid_t buffer,
	  size_t buffer_size)
  {
    if (action_entry_point != 0) {
      const paction* action = current.action->automaton->find_action (action_entry_point);
      if (action != 0) {
	scheduler::schedule (caction (action, parameter));
      }
      else {
	// BUG:  Automaton scheduled a bad action.
	kout << "aep = " << hexformat (action_entry_point) << endl;
	kassert (0);
      }
    }
    
    if (current.action->type == OUTPUT) {
      // Check the data produced by an output.
      if (value != 0) {
	if (value_size > LILY_LIMITS_MAX_VALUE_SIZE || (value_size != 0 && !current.action->automaton->verify_span (value, value_size))) {
	  // BUG:  The automaton produced a bad copy value.
	  kassert (0);
	}
      }
      else {
	// No copy value was produced.
	// We know that value = 0.
	// Rectify the size.
	value_size = 0;
      }
      
      const size_t s = current.action->automaton->buffer_size (buffer);
      if (s != static_cast<size_t> (-1)) {
	if (buffer_size > s) {
	  // BUG:  The action produced a value buffer but claims the size is larger than the size in memory.
	  kassert (0);
	}
      }
      else {
	// No buffer was produced.
	// Rectify the buffer and size.
	buffer = -1;
	buffer_size = 0;
      }
    }
    
    scheduler::finish (value, value_size, buffer, buffer_size);
  }

  aid_t
  create (const void* automaton_buffer,
	  size_t automaton_size)
  {
    // BUG
    kassert (0);
    // const automaton* a = create_automaton (reinterpret_cast<logical_address_t> (automaton_buffer), reinterpret_cast<logical_address_t> (automaton_buffer) + automaton_size);
    // return a->aid ();
  }

  void
  bind (void)
  {
    // BUG
    kassert (0);
  }

  void
  loose (void)
  {
    // BUG
    kassert (0);
  }

  void
  destroy (void)
  {
    // BUG
    kassert (0);
  }

  // The automaton is requesting that the physical memory from [source, source + size) appear at [destination, destination + size) in its address space.
  // The destination and source are aligned to a page boundary and the size is rounded up to a multiple of the page size.
  // Map can fail for the following reasons:
  // *  The automaton is not privileged.
  // *  The low order bits of the destination and source do not agree when interpretted using the page size.
  // *  Size is zero.
  // *  Part of the source lies outside the regions of memory marked for memory-mapped I/O.
  // *  Part of the source is already claimed by some other automaton.  (Mapping the same region twice is an error.)
  // *  The destination address is not available.
  pair<int, int>
  map (automaton* a,
       const void* destination,
       const void* source,
       size_t size)
  {
    logical_address_t const destination_begin = align_down (reinterpret_cast<logical_address_t> (destination), PAGE_SIZE);
    logical_address_t const destination_end = align_up (reinterpret_cast<logical_address_t> (destination) + size, PAGE_SIZE);
    physical_address_t const source_begin = align_down (reinterpret_cast<physical_address_t> (source), PAGE_SIZE);
    physical_address_t const source_end = align_up (reinterpret_cast<physical_address_t> (source) + size, PAGE_SIZE);

    if (!a->privileged ()) {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }

    if ((reinterpret_cast<logical_address_t> (destination) & (PAGE_SIZE - 1)) != (reinterpret_cast<physical_address_t> (source) & (PAGE_SIZE - 1))) {
      return make_pair (-1, LILY_SYSCALL_EINVAL);
    }

    if (size == 0) {
      return make_pair (-1, LILY_SYSCALL_EINVAL);
    }

    // I assume that all memory mapped I/O involves the region between 0 and 0x00100000.
    if (source_end > ONE_MEGABYTE) {
      return make_pair (-1, LILY_SYSCALL_EADDRNOTAVAIL);
    }

    for (physical_address_t address = source_begin; address != source_end; address += PAGE_SIZE) {
      if (mmapped_frames_[physical_address_to_frame (address)]) {
	return make_pair (-1, LILY_SYSCALL_EADDRINUSE);
      }
    }

    if (!a->vm_area_is_free (destination_begin, destination_end)) {
      return make_pair (-1, LILY_SYSCALL_EADDRINUSE);
    }

    mapped_area* area = new mapped_area (destination_begin,
					 destination_end,
					 source_begin,
					 source_end);
    a->insert_mapped_area (area);

    physical_address_t pa = source_begin;
    for (logical_address_t la = destination_begin; la != destination_end; la += PAGE_SIZE, pa += PAGE_SIZE) {
      mmapped_frames_[physical_address_to_frame (pa)] = true;
      vm::map (la, physical_address_to_frame (pa), vm::USER, vm::MAP_READ_WRITE, true);
    }

    // Success.
    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

}
