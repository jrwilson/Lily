#include "rts.hpp"
#include "kassert.hpp"
#include "vm.hpp"
#include "automaton.hpp"
//#include "scheduler.hpp"
//#include "elf.hpp"

namespace rts {
  
  // // For creating automata and allocating them an aid.
  // static aid_t current_aid_;
  // typedef std::unordered_map<aid_t, automaton*> aid_map_type;
  // static aid_map_type aid_map_;

  // struct map_op {
  //   logical_address_t logical_address;
  //   frame_t frame;
  //   vm::map_mode_t map_mode;

  //   map_op (logical_address_t la,
  // 	    frame_t f,
  // 	    vm::map_mode_t m) :
  //     logical_address (la),
  //     frame (f),
  //     map_mode (m)
  //   { }
  // };

  // // A list of frames used when creating automata.
  // typedef std::vector<map_op> frame_list_type;
  // static frame_list_type frame_list_;

  // typedef std::vector<std::pair<logical_address_t, logical_address_t> > clear_list_type;
  // static clear_list_type clear_list_;

  // static automaton*
  // create_automaton (logical_address_t text_begin,
  // 		    logical_address_t text_end)
  // {
  //   // Create the automaton.

  //   // Allocate a frame.
  //   frame_t frame = frame_manager::alloc ();
  //   // Map the page directory.
  //   vm::map (vm::get_stub1 (), frame, vm::USER, vm::MAP_READ_WRITE);
  //   vm::page_directory* pd = reinterpret_cast<vm::page_directory*> (vm::get_stub1 ());
  //   // Initialize the page directory with a copy of the kernel.
  //   // Since the second argument is vm::SUPERVISOR, the automaton cannot access the paging area, i.e., manipulate virtual memory.
  //   // If the third argument is vm::USER, the automaton gains access to kernel data.
  //   // If the third argument is vm::SUPERVISOR, the automaton does not gain access to kernel data.
  //   new (pd) vm::page_directory (frame, vm::SUPERVISOR, vm::SUPERVISOR);
  //   // Unmap.
  //   vm::unmap (vm::get_stub1 ());
  //   // Drop the reference count from allocation.
  //   size_t count = frame_manager::decref (frame);
  //   kassert (count == 1);

  //   // Generate an id.
  //   aid_t aid = current_aid_;
  //   while (aid_map_.find (aid) != aid_map_.end ()) {
  //     aid = std::max (aid + 1, 0); // Handles overflow.
  //   }
  //   current_aid_ = std::max (aid + 1, 0);
    
  //   // Create the automaton and insert it into the map.
  //   automaton* a = new automaton (aid, frame);
  //   aid_map_.insert (std::make_pair (aid, a));
    
  //   // Add to the scheduler.
  //   scheduler::add_automaton (a);

  //   // Parse the file.
  //   elf::parser hp;
  //   if (!hp.parse (a, text_begin, text_end)) {
  //     // TODO
  //     kassert (0);
  //   }

  //   // Add the actions to the automaton.
  //   for (elf::parser::action_iterator pos = hp.action_begin (); pos != hp.action_end (); ++pos) {
  //     if (!a->add_action (*pos)) {
  //   	// TODO:  The action conflicts with an existing action.  Be sure to delete the rest of the pointers.
  //   	kassert (0);
  //     }
  //   }

  //   // Create the memory map for the automaton.
  //   frame_list_.clear ();
  //   clear_list_.clear ();
  //   for (elf::parser::program_header_iterator e = hp.program_header_begin (); e != hp.program_header_end (); ++e) {
  //     vm::map_mode_t map_mode = ((e->permissions & elf::WRITE) != 0) ? vm::MAP_COPY_ON_WRITE : vm::MAP_READ_ONLY;
      
  //     size_t s;
  //     // Initialized data.
  //     for (s = 0; s < e->file_size; s += PAGE_SIZE) {
  //   	frame_list_.push_back (map_op (e->virtual_address + s, vm::logical_address_to_frame (text_begin + e->offset + s), map_mode));
  //     }
      
  //     // Clear the tiny region between the end of initialized data and the first unitialized page.
  //     if (e->file_size < e->memory_size) {
  //   	logical_address_t begin = e->virtual_address + e->file_size;
  //   	logical_address_t end = e->virtual_address + e->memory_size;
  //   	clear_list_.push_back (std::make_pair (begin, end));
  //     }
      
  //     // Uninitialized data.
  //     for (; s < e->memory_size; s += PAGE_SIZE) {
  //   	frame_list_.push_back (map_op (e->virtual_address + s, vm::zero_frame (), map_mode));
  //     }
      
  //     vm_area_base* area = new vm_area_base (e->virtual_address, e->virtual_address + e->memory_size);
  //     if (!a->insert_vm_area (area)) {
  //   	// TODO:  The area conflicts with the existing memory map.
  //   	kassert (0);
  //     }
  //   }

  //   // Switch to the automaton.
  //   physical_address_t old = vm::switch_to_directory (a->page_directory_physical_address ());
    
  //   // We can only use data in the kernel, i.e., we can't use automaton_text or hp.
    
  //   // Map all the frames.
  //   for (frame_list_type::const_iterator pos = frame_list_.begin ();
  //   	 pos != frame_list_.end ();
  //   	 ++pos) {
  //     vm::map (pos->logical_address, pos->frame, vm::USER, pos->map_mode);
  //   }

  //   // Clear.
  //   for (clear_list_type::const_iterator pos = clear_list_.begin ();
  //   	 pos != clear_list_.end ();
  //   	 ++pos) {
  //     memset (reinterpret_cast<void*> (pos->first), 0, pos->second - pos->first);
  //   }

  //   // Switch back.
  //   vm::switch_to_directory (old);

  //   return a;
  // }

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
      vm::map (address, frame, vm::USER, vm::MAP_READ_ONLY);
      // Drop the reference count back to 1.
      size_t count = frame_manager::decref (frame);
      kassert (count == 1);
    }

    automaton* child = create_automaton (text_begin, text_end);

    // Unmap the program text.
    for (address = text_begin; address < text_end; address += PAGE_SIZE) {
      vm::unmap (address);
    }

    // Create a buffer to contain the initializationd data.
    bid_t data_bid = child->buffer_create (0);
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
    const paction* action = child->find_action (std::string ("init"));

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

    scheduler::schedule (caction (action, data_bid));

    // Start the scheduler.  Doesn't return.
    scheduler::finish (0, 0, -1, 0);
  }

  aid_t
  create (const void* automaton_buffer,
	  size_t automaton_size)
  {
    kassert (0);
    // const automaton* a = create_automaton (reinterpret_cast<logical_address_t> (automaton_buffer), reinterpret_cast<logical_address_t> (automaton_buffer) + automaton_size);
    // return a->aid ();
  }

  void
  bind (void)
  {
    // TODO
    kassert (0);
  }

  void
  loose (void)
  {
    // TODO
    kassert (0);
  }

  void
  destroy (void)
  {
    // TODO
    kassert (0);
  }

}
