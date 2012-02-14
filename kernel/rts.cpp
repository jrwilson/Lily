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
  
  // Bitset marking which I/O ports are reserved.
  static bitset<65536> reserved_ports_;

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

  static automaton* registry = 0;

  static automaton*
  create_automaton (buffer* text,
		    size_t text_size,
		    bool privileged)
  {
    // Map the text of the initial automaton just after low memory.
    const logical_address_t text_begin = INITIAL_LOGICAL_LIMIT - KERNEL_VIRTUAL_BASE;
    const logical_address_t text_end = text_begin + text_size;
    text->map_begin (text_begin);

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
      switch ((*pos)->action_number) {
      case LILY_ACTION_NO_ACTION:
    	// BUG:  The action number LILY_ACTION_NO_ACTION is reserved.
   	kassert (0);
	break;
      case LILY_ACTION_INIT:
	if ((*pos)->type != LILY_ACTION_SYSTEM_INPUT) {
	  // BUG:  These must have the correct type.
	  kassert (0);
	}
	// Fall through.
      default:
	if (!a->add_action (*pos)) {
	  // BUG:  The action conflicts with an existing action.  Be sure to delete the rest of the pointers.
	  kassert (0);
	}
	break;
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

    // Unmap the text.
    text->unmap ();

    return a;
  }

  void
  create_init_automaton (frame_t automaton_frame_begin,
			 size_t automaton_size,
			 frame_t data_frame_begin,
			 size_t data_size)
  {
    // Create a buffer containing the text of the initial automaton.
    const size_t automaton_frame_end = automaton_frame_begin + align_up (automaton_size, PAGE_SIZE) / PAGE_SIZE;
    buffer text (0);
    for (size_t frame = automaton_frame_begin; frame != automaton_frame_end; ++frame) {
      text.append_frame (frame);
      // Drop the reference count.
      size_t count = frame_manager::decref (frame);
      kassert (count == 1);
    }

    // Create the automaton.
    automaton* child = create_automaton (&text, automaton_size, true);

    // Create a buffer to contain the initial data.
    buffer* data_buffer = new buffer (0);
    kassert (data_buffer != 0);
    const size_t data_frame_end = data_frame_begin + align_up (data_size, PAGE_SIZE) / PAGE_SIZE;
    for (frame_t frame = data_frame_begin; frame != data_frame_end; ++frame) {
      data_buffer->append_frame (frame);
      // Drop the reference count.
      size_t count = frame_manager::decref (frame);
      kassert (count == 1);
    }

    // Schedule the init action.
    const paction* action = child->find_action (LILY_ACTION_INIT);

    if (action == 0) {
      kout << "The initial automaton does not contain an init action.  Halting." << endl;
      halt ();
    }

    scheduler::schedule (caction (action, child->aid (), data_buffer));

    // Start the scheduler.  Doesn't return.
    scheduler::finish (-1, LILY_SYSCALL_FINISH_NO);
  }

  // The automaton has finished the current action.
  // The parameters are interrupted as follows:
  //
  //   action_number and parameter can be used to schedule another action.
  //   These are ignored if the action does not exist.
  //
  //   buffer and flag can be used to manipulate buffers.
  //   The buffer generated by a successful output action is transferred to all bound inputs.
  //
  //   An output action can end in one of three ways conveyed by flags: NO, YES, or DESTROY.
  //   NO means that the input actions should not be executed, i.e., the precondition was false.
  //   YES means that the input actions should be executed.
  //   If the buffer parameter points to a valid buffer, then the inputs are given a copy of the buffer.
  //   DESTROY is equivalent to YES except that the buffer is destroyed.
  //   -1 always referes to a buffer that doesn't exist.
  //
  //   Input actions may receive buffers from output actions as described above.
  //   The buffer will always be -1 and the buffer_size will always be 0 if there is no buffer.
  //   The automaton should DESTROY the buffer if it does not which to retain it for further processing.
  //   Finishing an input action with NO or YES retains the buffer.
  //   The buffer supplied to the finish command does not need to be the buffer that was supplied to the input action.
  //
  //   Internal actions are similar to input actions.
  void
  finish (const caction& current,
	  ano_t action_number,
	  int parameter,
	  bd_t bd,
	  int flags)
  {
    automaton* a = current.action->automaton;
    const paction* action = a->find_action (action_number);
    if (action != 0) {
      switch (action->type) {
      case OUTPUT:
      case INTERNAL:
	scheduler::schedule (caction (action, parameter));
	break;
      case INPUT:
      case SYSTEM_INPUT:
	// Can't schedule non-local actions.
	kassert (0);
	break;
      }
    }

    switch (current.action->type) {
    case INPUT:
    case INTERNAL:
    case SYSTEM_INPUT:
      // They wish to destroy a buffer.
      if (flags == LILY_SYSCALL_FINISH_DESTROY) {
	// We don't care if this fails.  It just means the buffer didn't exist.
	a->buffer_destroy (bd);
      }
      bd = -1;
      flags = LILY_SYSCALL_FINISH_NO;
      break;
    case OUTPUT:
      {
	buffer* b = current.action->automaton->lookup_buffer (bd);
	if (b == 0) {
	  // The buffer does not exist.
	  bd = -1;
	  if (flags == LILY_SYSCALL_FINISH_DESTROY) {
	    // Can't destroy a buffer that doesn't exist.
	    flags = LILY_SYSCALL_FINISH_YES;
	  }
	}
      }
      break;
    }

    scheduler::finish (bd, flags);
  }

  pair<aid_t, int>
  create (automaton* a,
	  bd_t text_bd,
	  size_t text_buffer_size,
	  bool retain_privilege,
	  bd_t data_bd)
  {
    // Find the text buffer.
    buffer* text_buffer = a->lookup_buffer (text_bd);

    if (text_buffer == 0) {
      // Buffer does not exist.
      return make_pair (-1, LILY_SYSCALL_EBDDNE);
    }

    if (!(text_buffer_size <= text_buffer->capacity ())) {
      // They claim that the buffer is bigger than it really is.
      return make_pair (-1, LILY_SYSCALL_EBDSIZE);
    }

    // Synchronize the buffer so the frames listed in the buffer are correct.
    text_buffer->sync (0, text_buffer_size);

    // If the buffer is not mapped, there are no problems.
    // If the buffer is mapped, we have three options:
    // 1.  Copy the buffer.
    // 2.  Unmap the buffer and then map it back at it original location.
    // 3.  Save the location of the buffer and temporatily override it.
    // Option 3 has the least overhead so that's what we'll do.

    logical_address_t const begin = text_buffer->begin ();
    logical_address_t const end = text_buffer->end ();

    text_buffer->override (0, 0);

    // Switch to the kernel page directory.  So we can map the text of the automaton.
    physical_address_t old = vm::switch_to_directory (vm::get_kernel_page_directory_physical_address ());

    // Create the automaton.
    const automaton* child = create_automaton (text_buffer, text_buffer_size, retain_privilege && a->privileged ());

    // Switch back.
    vm::switch_to_directory (old);

    text_buffer->override (begin, end);

    // Find the data buffer.
    buffer* data_buffer = a->lookup_buffer (data_bd);
    if (data_buffer != 0) {
      // Synchronize the buffer so the frames listed in the buffer are correct.
      data_buffer->sync (0, data_buffer->capacity ());
    }

    // Schedule the init action.
    const paction* action = child->find_action (LILY_ACTION_INIT);
    if (action != 0) {
      if (data_buffer != 0) {
	scheduler::schedule (caction (action, child->aid (), new buffer (*data_buffer)));
      }
      else {
	scheduler::schedule (caction (action, child->aid ()));
      }
    }
    
    return make_pair (child->aid (), LILY_SYSCALL_ESUCCESS);
  }

  pair<bid_t, int>
  bind (automaton* a,
	aid_t output_aid,
	ano_t output_ano,
	int output_parameter,
	aid_t input_aid,
	ano_t input_ano,
	int input_parameter)
  {
    aid_map_type::const_iterator output_pos = aid_map_.find (output_aid);
    if (output_pos == aid_map_.end ()) {
      // BUG:  Output automaton DNE.
      kassert (0);
    }
    automaton* output_automaton = output_pos->second;

    aid_map_type::const_iterator input_pos = aid_map_.find (input_aid);
    if (input_pos == aid_map_.end ()) {
      // BUG:  Input automaton DNE.
      kassert (0);
    }
    automaton* input_automaton = input_pos->second;

    if (output_automaton == input_automaton) {
      // BUG:  The output and input automata must be different.
      kassert (0);
    }

    // Check the output action dynamically.
    const paction* output_action = output_automaton->find_action (output_ano);
    if (output_action == 0 ||
  	output_action->type != OUTPUT) {
      // BUG:  Output action does not exist or has the wrong type.
      kassert (0);
    }

    // Check the input action dynamically.
    const paction* input_action = input_automaton->find_action (input_ano);
    if (input_action == 0 ||
  	input_action->type != INPUT) {
      // BUG:  Input action does not exist or has the wrong type.
      kassert (0);
    }

    // Adust the parameters.
    switch (output_action->parameter_mode) {
    case NO_PARAMETER:
      output_parameter = 0;
      break;
    case PARAMETER:
      break;
    case AUTO_PARAMETER:
      output_parameter = input_automaton->aid ();
      break;
    }

    switch (input_action->parameter_mode) {
    case NO_PARAMETER:
      input_parameter = 0;
      break;
    case PARAMETER:
      break;
    case AUTO_PARAMETER:
      input_parameter = output_automaton->aid ();
      break;
    }

    // Form complete actions.
    caction oa (output_action, output_parameter);
    caction ia (input_action, input_parameter);

    // Check that the input action is not bound.
    // (Also checks that the binding does not exist.)
    if (input_automaton->is_input_bound (ia)) {
      // BUG
      kassert (0);
    }

    // Check that the output action is not bound to an action in the input automaton.
    if (output_automaton->is_output_bound_to_automaton (oa, input_automaton)) {
      // BUG
      kassert (0);
    }

    // Bind.
    output_automaton->bind_output (oa, ia, a);
    input_automaton->bind_input (oa, ia, a);
    bid_t bid = a->bind (oa, ia);

    // Schedule the output action.
    scheduler::schedule (caction (output_action, output_parameter));

    return make_pair (bid, LILY_SYSCALL_ESUCCESS);
  }

  // The automaton wants to receive a notification via action_number when the automaton corresponding to aid is destroyed.
  // The automaton must have an action for receiving the event.
  // The automaton corresponding to aid must exist.
  // Not an error if already subscribed.
  pair<int, int>
  subscribe_destroyed (automaton* a,
		       ano_t action_number,
		       aid_t aid)
  {
    const paction* action = a->find_action (action_number);
    if (action == 0 || action->type != SYSTEM_INPUT || action->parameter_mode != PARAMETER) {
      return make_pair (-1, LILY_SYSCALL_EBADANO);
    }

    aid_map_type::iterator pos = aid_map_.find (aid);
    if (pos == aid_map_.end ()) {
      return make_pair (-1, LILY_SYSCALL_EAIDDNE);
    }

    a->add_subscription (pos->second);
    pos->second->add_subscriber (a);

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
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

  // The automaton is requesting to become the registry.
  // Succeed unless the registry is already set.
  pair<int, int>
  set_registry (aid_t aid)
  {
    aid_map_type::iterator pos = aid_map_.find (aid);
    if (pos == aid_map_.end ()) {
      return make_pair (-1, LILY_SYSCALL_EAIDDNE);
    }

    if (registry != 0) {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }

    registry = pos->second;
    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  pair<aid_t, int>
  get_registry (void)
  {
    if (registry != 0) {
      return make_pair (registry->aid (), LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EAIDDNE);
    }
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
      return make_pair (-1, LILY_SYSCALL_EALIGN);
    }

    if (size == 0) {
      return make_pair (-1, LILY_SYSCALL_ESIZE);
    }

    // I assume that all memory mapped I/O involves the region between 0 and 0x00100000.
    if (source_end > ONE_MEGABYTE) {
      return make_pair (-1, LILY_SYSCALL_ESRCNA);
    }

    for (physical_address_t address = source_begin; address != source_end; address += PAGE_SIZE) {
      if (mmapped_frames_[physical_address_to_frame (address)]) {
	return make_pair (-1, LILY_SYSCALL_ESRCINUSE);
      }
    }

    if (!a->vm_area_is_free (destination_begin, destination_end)) {
      return make_pair (-1, LILY_SYSCALL_EDSTINUSE);
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

  // The automaton is requesting access to the specified I/O port.
  pair<int, int>
  reserve_port (automaton* a,
		unsigned short port)
  {
    if (!a->privileged ()) {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }

    if (reserved_ports_[port]) {
      // Port is already reserved.
      return make_pair (-1, LILY_SYSCALL_EPORTINUSE);
    }

    // Reserve the port.
    a->reserve_port (port);
    reserved_ports_[port] = true;

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

}
