#include "rts.hpp"
#include "kassert.hpp"
#include "scheduler.hpp"
#include "system_automaton_private.hpp"
#include "elf.hpp"

// Symbols to build memory maps.
extern int text_begin;
extern int text_end;
extern int data_begin;
extern int data_end;

namespace rts {
  
  // Heap location for system automata.
  // Starting at 4MB allow us to use the low page table of the kernel.
  static const logical_address_t SYSTEM_HEAP_BEGIN = FOUR_MEGABYTES;
  
  // The system automaton.
  automaton* system_automaton;

  // For loading the first automaton.
  bid_t automaton_bid;
  size_t automaton_size;
  bid_t data_bid;
  size_t data_size;

  // For creating automata and allocating them an aid.
  static aid_t current_aid_;
  typedef std::unordered_map<aid_t, automaton*, std::hash<aid_t>, std::equal_to<aid_t>, kernel_allocator<std::pair<const aid_t, automaton*> > > aid_map_type;
  static aid_map_type aid_map_;

  static automaton*
  create_automaton (vm::page_privilege_t kernel_map)
  {
    // Allocate a frame.
    frame_t frame = frame_manager::alloc ();
    // Map the page directory.
    vm::map (vm::get_stub1 (), frame, vm::USER, vm::MAP_READ_WRITE);
    vm::page_directory* pd = reinterpret_cast<vm::page_directory*> (vm::get_stub1 ());
    // Initialize the page directory with a copy of the kernel.
    // Since the second argument is vm::SUPERVISOR, the automaton cannot access the paging area, i.e., manipulate virtual memory.
    // If the third argument is vm::USER, the automaton gains access to kernel data.
    // If the third argument is vm::SUPERVISOR, the automaton does not gain access to kernel data.
    new (pd) vm::page_directory (frame, vm::SUPERVISOR, kernel_map);
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
    automaton* a = new (kernel_alloc ()) automaton (aid, frame);
    aid_map_.insert (std::make_pair (aid, a));
    
    // Add to the scheduler.
    scheduler::add_automaton (a);

    return a;
  }

  static bool
  bind_actions (automaton* output_automaton,
		const kstring& output_action_name,
		aid_t output_parameter,
		automaton* input_automaton,
		const kstring& input_action_name,
		aid_t input_parameter,
		automaton* owner)
  {
    kassert (output_automaton != 0);
    kassert (input_automaton != 0);
    kassert (owner != 0);

    if (output_automaton == input_automaton) {
      // The output and input automata must be different.
      return false;
    }

    // Check the output action dynamically.
    const paction* output_action = output_automaton->find_action (output_action_name);
    if (output_action == 0 ||
	output_action->type != OUTPUT) {
      // Output action does not exist or has the wrong type.
      return false;
    }

    // Check the input action dynamically.
    const paction* input_action = input_automaton->find_action (input_action_name);
    if (input_action == 0 ||
	input_action->type != INPUT) {
      // Input action does not exist or has the wrong type.
      return false;
    }

    // Check the descriptions.
    if (output_action->compare_method != input_action->compare_method) {
      return false;
    }

    switch (output_action->compare_method) {
    case NO_COMPARE:
      // Okay.  I won't.
      break;
    case EQUAL:
      // Simple string equality.
      if (output_action->description != input_action->description) {
	return false;
      }
      break;
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
      return false;
    }

    // Check that the output action is not bound to an action in the input automaton.
    if (output_automaton->is_output_bound_to_automaton (oa, input_automaton)) {
      return false;
    }

    // Bind.
    output_automaton->bind_output (oa, ia, owner);
    input_automaton->bind_input (oa, ia, owner);
    owner->bind (oa, ia);

    return true;
  }

  static void
  checked_schedule (automaton* a,
		    const void* aep,
		    aid_t p = 0)
  {
    const paction* action = a->find_action (aep);
    kassert (action != 0);
    scheduler::schedule (caction (action, p));
  }

  void
  create_system_automaton (buffer* automaton_buffer,
			   size_t automaton_size,
			   buffer* data_buffer,
			   size_t data_size)
  {
    // Create the automaton.
    // Automaton can access kernel data.  (That is where its code/data reside.)
    system_automaton = create_automaton (vm::USER);
    
    // Create the memory map.
    vm_area_base* area;
    bool r;
    
    // Text.
    area = new (kernel_alloc ()) vm_area_base (reinterpret_cast<logical_address_t> (&text_begin),
					       reinterpret_cast<logical_address_t> (&text_end));
    r = system_automaton->insert_vm_area (area);
    kassert (r);
    
    // Data.
    area = new (kernel_alloc ()) vm_area_base (reinterpret_cast<logical_address_t> (&data_begin),
					       reinterpret_cast<logical_address_t> (&data_end));
    r = system_automaton->insert_vm_area (area);
    kassert (r);
    
    // Heap.
    area = new (kernel_alloc ()) vm_area_base (SYSTEM_HEAP_BEGIN, SYSTEM_HEAP_BEGIN);
    r = system_automaton->insert_heap_area (area);
    kassert (r);
    
    // Add the actions.
    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_FIRST_NAME, SA_FIRST_DESCRIPTION, system_automaton::first_traits::compare_method, system_automaton::first_traits::action_type, reinterpret_cast<const void*> (&::first), system_automaton::first_traits::parameter_mode));
    kassert (r);

    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_CREATE_REQUEST_NAME, SA_CREATE_REQUEST_DESCRIPTION, system_automaton::create_request_traits::compare_method, system_automaton::create_request_traits::action_type, reinterpret_cast<const void*> (&::create_request), system_automaton::create_request_traits::parameter_mode));
    kassert (r);
    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_CREATE_NAME, SA_CREATE_DESCRIPTION, system_automaton::create_traits::compare_method, system_automaton::create_traits::action_type, reinterpret_cast<const void*> (&::create), system_automaton::create_traits::parameter_mode));
    kassert (r);
    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_INIT_NAME, SA_INIT_DESCRIPTION, system_automaton::init_traits::compare_method, system_automaton::init_traits::action_type, reinterpret_cast<const void*> (&::init), system_automaton::init_traits::parameter_mode));
    kassert (r);
    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_CREATE_RESPONSE_NAME, SA_CREATE_RESPONSE_DESCRIPTION, system_automaton::create_response_traits::compare_method, system_automaton::create_response_traits::action_type, reinterpret_cast<const void*> (&::create_response), system_automaton::create_response_traits::parameter_mode));
    kassert (r);

    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_BIND_REQUEST_NAME, SA_BIND_REQUEST_DESCRIPTION, system_automaton::bind_request_traits::compare_method, system_automaton::bind_request_traits::action_type, reinterpret_cast<const void*> (&::bind_request), system_automaton::bind_request_traits::parameter_mode));
    kassert (r);
    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_BIND_NAME, SA_BIND_DESCRIPTION, system_automaton::bind_traits::compare_method, system_automaton::bind_traits::action_type, reinterpret_cast<const void*> (&::bind), system_automaton::bind_traits::parameter_mode));
    kassert (r);
    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_BIND_RESPONSE_NAME, SA_BIND_RESPONSE_DESCRIPTION, system_automaton::bind_response_traits::compare_method, system_automaton::bind_response_traits::action_type, reinterpret_cast<const void*> (&::bind_response), system_automaton::bind_response_traits::parameter_mode));
    kassert (r);

    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_LOOSE_REQUEST_NAME, SA_LOOSE_REQUEST_DESCRIPTION, system_automaton::loose_request_traits::compare_method, system_automaton::loose_request_traits::action_type, reinterpret_cast<const void*> (&::loose_request), system_automaton::loose_request_traits::parameter_mode));
    kassert (r);
    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_LOOSE_NAME, SA_LOOSE_DESCRIPTION, system_automaton::loose_traits::compare_method, system_automaton::loose_traits::action_type, reinterpret_cast<const void*> (&::loose), system_automaton::loose_traits::parameter_mode));
    kassert (r);
    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_LOOSE_RESPONSE_NAME, SA_LOOSE_RESPONSE_DESCRIPTION, system_automaton::loose_response_traits::compare_method, system_automaton::loose_response_traits::action_type, reinterpret_cast<const void*> (&::loose_response), system_automaton::loose_response_traits::parameter_mode));
    kassert (r);

    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_DESTROY_REQUEST_NAME, SA_DESTROY_REQUEST_DESCRIPTION, system_automaton::destroy_request_traits::compare_method, system_automaton::destroy_request_traits::action_type, reinterpret_cast<const void*> (&::destroy_request), system_automaton::destroy_request_traits::parameter_mode));
    kassert (r);
    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_DESTROY_NAME, SA_DESTROY_DESCRIPTION, system_automaton::destroy_traits::compare_method, system_automaton::destroy_traits::action_type, reinterpret_cast<const void*> (&::destroy), system_automaton::destroy_traits::parameter_mode));
    kassert (r);
    r = system_automaton->add_action (new (kernel_alloc ()) paction (system_automaton, SA_DESTROY_RESPONSE_NAME, SA_DESTROY_RESPONSE_DESCRIPTION, system_automaton::destroy_response_traits::compare_method, system_automaton::destroy_response_traits::action_type, reinterpret_cast<const void*> (&::destroy_response), system_automaton::destroy_response_traits::parameter_mode));
    kassert (r);
    
    // Bootstrap.
    checked_schedule (system_automaton, reinterpret_cast<const void*> (&::first));

    rts::automaton_bid = system_automaton->buffer_create (*automaton_buffer);
    rts::automaton_size = automaton_size;
    rts::data_bid = system_automaton->buffer_create (*data_buffer);
    rts::data_size = data_size;
  }

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
  typedef std::vector<map_op, kernel_allocator<map_op> > frame_list_type;
  static frame_list_type frame_list_;

  typedef std::vector<std::pair<logical_address_t, logical_address_t>, kernel_allocator<std::pair<logical_address_t, logical_address_t> > > clear_list_type;
  static clear_list_type clear_list_;

  void
  create (bid_t automaton_bid,
	  size_t automaton_size)
  {
    // Map the buffer containing the program text.
    const void* automaton_text = system_automaton->buffer_map (automaton_bid);
    kassert (automaton_text != 0);

    // Get the buffer as well so we can look at the frames.
    const buffer* buffer = system_automaton->lookup_buffer (automaton_bid);
    kassert (buffer != 0);

    // Create the automaton.
    automaton* child = create_automaton (vm::SUPERVISOR);

    // Parse the file.
    elf::parser hp;
    if (!hp.parse (child, automaton_text, automaton_size)) {
      // TODO
      kassert (0);
    }

    // Add the actions to the automaton.
    for (elf::parser::action_iterator pos = hp.action_begin (); pos != hp.action_end (); ++pos) {
      if (!child->add_action (*pos)) {
	// TODO:  The action conflicts with an existing action.  Be sure to delete the rest of the pointers.
	kassert (0);
      }
    }

    // Bind the system automaton to the new automaton.
    if (!bind_actions (system_automaton, SA_INIT_NAME, 0, child, SA_INIT_NAME, 0, system_automaton)) {
      // Couldn't bind to init.
      kassert (0);
    }

    kout << "Bind to system automaton" << endl;

    // Build a map from logical address to frame.
    // Build a list of areas that need to be cleared.
    // Create the memory map for the automaton.
    frame_list_.clear ();
    clear_list_.clear ();
    for (elf::parser::program_header_iterator e = hp.program_header_begin (); e != hp.program_header_end (); ++e) {
      vm::map_mode_t map_mode = ((e->permissions & elf::WRITE) != 0) ? vm::MAP_COPY_ON_WRITE : vm::MAP_READ_ONLY;
      
      size_t s;
      // Initialized data.
      for (s = 0; s < e->file_size; s += PAGE_SIZE) {
	frame_list_.push_back (map_op (e->virtual_address + s, vm::logical_address_to_frame (buffer->begin () + e->offset + s), map_mode));
      }
      
      // Clear the tiny region between the end of initialized data and the first unitialized page.
      if (e->file_size < e->memory_size) {
	logical_address_t begin = e->virtual_address + e->file_size;
	logical_address_t end = e->virtual_address + e->memory_size;
	clear_list_.push_back (std::make_pair (begin, end));
      }
      
      // Uninitialized data.
      for (; s < e->memory_size; s += PAGE_SIZE) {
	frame_list_.push_back (map_op (e->virtual_address + s, vm::zero_frame (), map_mode));
      }
      
      vm_area_base* area = new (kernel_alloc ()) vm_area_base (e->virtual_address, e->virtual_address + e->memory_size);
      if (!child->insert_vm_area (area)) {
	// TODO:  The area conflicts with the existing memory map.
	kassert (0);
      }
    }

    // Switch to the automaton.
    physical_address_t old = vm::switch_to_directory (child->page_directory_physical_address ());
    
    // We can only use data in the kernel, i.e., we can't use automaton_text or hp.
    
    // Map all the frames.
    for (frame_list_type::const_iterator pos = frame_list_.begin ();
    	 pos != frame_list_.end ();
    	 ++pos) {
      vm::map (pos->logical_address, pos->frame, vm::USER, pos->map_mode);
    }

    // Clear.
    for (clear_list_type::const_iterator pos = clear_list_.begin ();
    	 pos != clear_list_.end ();
    	 ++pos) {
      memset (reinterpret_cast<void*> (pos->first), 0, pos->second - pos->first);
    }

    // Switch back.
    vm::switch_to_directory (old);
    
    // Unmap the program text.
    system_automaton->buffer_unmap (automaton_bid);

    // TODO
    kassert (0);
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
