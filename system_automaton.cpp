/*
  File
  ----
  system_automaton.cpp
  
  Description
  -----------
  The system automaton.

  Authors:
  Justin R. Wilson
*/

#include "system_automaton.hpp"
#include "scheduler.hpp"
#include "aid_manager.hpp"
#include "fifo_scheduler.hpp"
#include "kassert.hpp"

#include "action_test_automaton.hpp"

// Symbols to build the kernel's memory map.
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

namespace system_automaton {

  // Heap location for system automata.
  static const void* const SYSTEM_HEAP_BEGIN = reinterpret_cast<const void*> (ONE_MEGABYTE);

  // Stack range for system automata.
  static const void* const SYSTEM_STACK_BEGIN = reinterpret_cast<const uint8_t*> (KERNEL_VIRTUAL_BASE) - 0x1000;
  static const void* const SYSTEM_STACK_END = KERNEL_VIRTUAL_BASE;

  // Stack for the system automaton.
  static const void* const SYSTEM_AUTOMATON_STACK_BEGIN = reinterpret_cast<const uint8_t*> (PAGING_AREA) - 0x1000;
  static const void* const SYSTEM_AUTOMATON_STACK_END = PAGING_AREA;
  
  static scheduler system_scheduler_;
  static aid_manager aid_manager_;
  automaton* system_automaton = 0;

  typedef fifo_scheduler<system_allocator> scheduler_type;
  static scheduler_type* scheduler_;

  struct schedule {
    void
    operator () () const;
  };
  
  typedef p_uv_output_action_traits<automaton*> init_traits;
  
  static bool
  init_precondition (automaton*)
  {
    return true;
  }
  
  static void
  init_effect (automaton*)
  {
    // Do nothing.
  }
  
  static void
  init (automaton* p)
  {
    output_action<init_traits> (p, scheduler_->remove<init_traits> (&init), init_precondition, init_effect, schedule (), scheduler_->finish ());
  }

  void
  schedule::operator() () const
  {
  }

  static void
  create_action_test ()
  {
    // Create automata to test actions.
    automaton* input_automaton;
    automaton* output_automaton;

    {    
      // First, create a new page directory.

      // Reserve some logical address space for the page directory.
      page_directory* pd = static_cast<page_directory*> (system_automaton->reserve (sizeof (page_directory)));
      kassert (pd != 0);
      // Allocate a frame.
      frame_t frame = frame_manager::alloc ();
      // Map the page directory.
      vm_manager::map (pd, frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE);
      // Initialize the page directory with a copy of the current page directory.
      pd->initialize_with_current (frame_to_physical_address (frame));
      // Unmap.
      vm_manager::unmap (pd);
      // Unreserve.
      system_automaton->unreserve (pd);
      // Switch to the new page directory.
      vm_manager::switch_to_directory (frame_to_physical_address (frame));

      // Second, create the automaton.
      input_automaton = new (system_alloc ()) automaton (aid_manager_.allocate (), descriptor_constants::RING0, vm_manager::page_directory_physical_address ());
      
      // Third, create the automaton's memory map.
      {
	vm_area_interface* area;
	bool r;

	// Text.
	area = new (system_alloc ()) vm_text_area (&text_begin, &text_end);
	r = input_automaton->insert_vm_area (area);
	kassert (r);
	
	// Read-only data.
	area = new (system_alloc ()) vm_rodata_area (&rodata_begin, &rodata_end);
	r = input_automaton->insert_vm_area (area);
	kassert (r);
	
	// Data.
	area = new (system_alloc ()) vm_data_area (&data_begin, &data_end);
	r = input_automaton->insert_vm_area (area);
	kassert (r);

	// Heap.
	vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
	r = input_automaton->insert_heap_area (heap_area);
	kassert (r);

	// Stack.
	vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
	r = input_automaton->insert_stack_area (stack_area);
	kassert (r);
      }
      
      // Fourth, add the actions.
      input_automaton->add_action<action_test::up_uv_input1_traits> (&action_test::up_uv_input1);
      input_automaton->add_action<action_test::up_uv_input2_traits> (&action_test::up_uv_input2);
      input_automaton->add_action<action_test::up_uv_input3_traits> (&action_test::up_uv_input3);
      input_automaton->add_action<action_test::up_v_input1_traits> (&action_test::up_v_input1);
      input_automaton->add_action<action_test::up_v_input2_traits> (&action_test::up_v_input2);
      input_automaton->add_action<action_test::up_v_input3_traits> (&action_test::up_v_input3);
      input_automaton->add_action<action_test::p_uv_input1_traits> (&action_test::p_uv_input1);
      input_automaton->add_action<action_test::p_uv_input2_traits> (&action_test::p_uv_input2);
      input_automaton->add_action<action_test::p_uv_input3_traits> (&action_test::p_uv_input3);
      input_automaton->add_action<action_test::p_v_input1_traits> (&action_test::p_v_input1);
      input_automaton->add_action<action_test::p_v_input2_traits> (&action_test::p_v_input2);
      input_automaton->add_action<action_test::p_v_input3_traits> (&action_test::p_v_input3);
      input_automaton->add_action<action_test::ap_uv_input1_traits> (&action_test::ap_uv_input1);
      input_automaton->add_action<action_test::ap_uv_input2_traits> (&action_test::ap_uv_input2);
      input_automaton->add_action<action_test::ap_uv_input3_traits> (&action_test::ap_uv_input3);
      input_automaton->add_action<action_test::ap_v_input1_traits> (&action_test::ap_v_input1);
      input_automaton->add_action<action_test::ap_v_input2_traits> (&action_test::ap_v_input2);
      input_automaton->add_action<action_test::ap_v_input3_traits> (&action_test::ap_v_input3);

      // Fifth, add the automaton to the system scheduler.
      system_scheduler_.add_automaton (input_automaton);
      
      // Sixth, bind.
    }


    kassert (0);

  //   {    
  //     /* First, create a new page directory. */
      
  //     /* Reserve some logical address space for the page directory. */
  //     page_directory* pd = static_cast<page_directory*> (system_automaton_->reserve (PAGE_SIZE));
  //     kassert (pd != 0);
  //     // Allocate a frame.
  //     frame_t frame = frame_manager::alloc ();
  //     /* Map the page directory. */
  //     vm_manager::map (pd, frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE);
  //     /* Initialize the page directory with a copy of the current page directory. */
  //     pd->initialize_with_current (frame_to_physical_address (frame));
  //     /* Unmap. */
  //     vm_manager::unmap (pd);
  //     /* Unreserve. */
  //     system_automaton_->unreserve (pd);
      
  //     /* Switch to the new page directory. */
  //     vm_manager::switch_to_directory (frame_to_physical_address (frame));
      
  //     /* Second, create the automaton. */
  //     output_automaton = new (alloc_) automaton_type (alloc_, aid_manager_->allocate (), descriptor_constants::RING0, frame_to_physical_address (frame), vm_manager::page_directory_logical_address (), vm_manager::page_directory_logical_address (), paging_constants::SUPERVISOR);
      
  //     /* Third, create the automaton's memory map. */
  //     {
  // 	//Reserve low memory.
  // 	bool r = output_automaton->insert_vm_area (new (alloc_) vm_reserved_area (0, ONE_MEGABYTE));
  // 	kassert (r);
	
  // 	// Program text.
  // 	r = output_automaton->insert_vm_area (new (alloc_) vm_text_area (&text_begin, &text_end, paging_constants::SUPERVISOR));
  // 	kassert (r);

  // 	// Program read-only data.
  // 	r = output_automaton->insert_vm_area (new (alloc_) vm_rodata_area (&rodata_begin, &rodata_end, paging_constants::SUPERVISOR));
  // 	kassert (r);

  // 	// Program data.
  // 	r = output_automaton->insert_vm_area (new (alloc_) vm_data_area (&data_begin, &data_end, paging_constants::SUPERVISOR));
  // 	kassert (r);
	
  // 	// TODO: Reserve additional memory to prevent corrupt of the heap.
	
  // 	/* Add a stack area. */
  // 	vm_stack_area* stack = new (alloc_) vm_stack_area (static_cast<const uint8_t*> (output_automaton->stack_pointer ()) - SYSTEM_STACK_SIZE,
  // 							   output_automaton->stack_pointer (),
  // 							   paging_constants::SUPERVISOR);
  // 	r = output_automaton->insert_vm_area (stack);
  // 	kassert (r);

  // 	/* Back with physical pages.  See previous comment about the stack. */
  // 	stack->back_with_frames ();
  //     }
      
  //     /* Fourth, add the actions. */
  //     output_automaton->add_action<action_test::init_traits> (&action_test::init);
  //     output_automaton->add_action<action_test::up_uv_output_traits> (&action_test::up_uv_output);
  //     output_automaton->add_action<action_test::up_v_output_traits> (&action_test::up_v_output);
  //     output_automaton->add_action<action_test::p_uv_output_traits> (&action_test::p_uv_output);
  //     output_automaton->add_action<action_test::p_v_output_traits> (&action_test::p_v_output);
  //     output_automaton->add_action<action_test::ap_uv_output_traits> (&action_test::ap_uv_output);
  //     output_automaton->add_action<action_test::ap_v_output_traits> (&action_test::ap_v_output);

  //     output_automaton->add_action<action_test::up_internal_traits> (&action_test::up_internal);
  //     output_automaton->add_action<action_test::p_internal_traits> (&action_test::p_internal);

  //     /* Fifth, add the automaton to the system scheduler. */
  //     system_scheduler_->add_automaton (output_automaton);
      
  //     /* Sixth, bind. */
  //     binding_manager_->bind<init_traits,
  // 	action_test::init_traits> (system_automaton_, &init, output_automaton,
  // 				   output_automaton, &action_test::init);
  //     system_scheduler_->schedule_action (system_automaton_, reinterpret_cast<const void*> (&init), aid_cast (output_automaton));

  //     binding_manager_->bind<action_test::up_uv_output_traits,
  // 	action_test::up_uv_input1_traits> (output_automaton, &action_test::up_uv_output,
  // 					   input_automaton, &action_test::up_uv_input1);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::up_uv_output), 0);

  //     binding_manager_->bind<action_test::p_uv_output_traits,
  // 	action_test::up_uv_input2_traits> (output_automaton, &action_test::p_uv_output, action_test::p_uv_output_parameter,
  // 					   input_automaton, &action_test::up_uv_input2);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::p_uv_output), aid_cast (action_test::p_uv_output_parameter));
      
  //     action_test::ap_uv_output_parameter = input_automaton->aid ();
  //     binding_manager_->bind<action_test::ap_uv_output_traits,
  // 	action_test::up_uv_input3_traits> (output_automaton, &action_test::ap_uv_output, action_test::ap_uv_output_parameter,
  // 					   input_automaton, &action_test::up_uv_input3);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::ap_uv_output), aid_cast (action_test::ap_uv_output_parameter));

  //     binding_manager_->bind<action_test::up_v_output_traits,
  // 	action_test::up_v_input1_traits> (output_automaton, &action_test::up_v_output,
  // 					  input_automaton, &action_test::up_v_input1);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::up_v_output), 0);
      
  //     binding_manager_->bind<action_test::p_v_output_traits,
  //     	action_test::up_v_input2_traits> (output_automaton, &action_test::p_v_output, action_test::p_v_output_parameter,
  //     					  input_automaton, &action_test::up_v_input2);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::p_v_output), aid_cast (action_test::p_v_output_parameter));
      
  //     action_test::ap_v_output_parameter = input_automaton->aid ();
  //     binding_manager_->bind<action_test::ap_v_output_traits,
  //     	action_test::up_v_input3_traits> (output_automaton, &action_test::ap_v_output, action_test::ap_v_output_parameter,
  //     					  input_automaton, &action_test::up_v_input3);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::ap_v_output), aid_cast (action_test::ap_v_output_parameter));

  //     binding_manager_->bind<action_test::up_uv_output_traits,
  //     	action_test::p_uv_input1_traits> (output_automaton, &action_test::up_uv_output,
  // 					  input_automaton, &action_test::p_uv_input1, action_test::p_uv_input1_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::up_uv_output), 0);
      
  //     binding_manager_->bind<action_test::p_uv_output_traits,
  //     	action_test::p_uv_input2_traits> (output_automaton, &action_test::p_uv_output, action_test::p_uv_output_parameter,
  // 					  input_automaton, &action_test::p_uv_input2, action_test::p_uv_input2_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::p_uv_output), aid_cast (action_test::p_uv_output_parameter));
      
  //     action_test::ap_uv_output_parameter = input_automaton->aid ();
  //     binding_manager_->bind<action_test::ap_uv_output_traits,
  //     	action_test::p_uv_input3_traits> (output_automaton, &action_test::ap_uv_output, action_test::ap_uv_output_parameter,
  // 					  input_automaton, &action_test::p_uv_input3, action_test::p_uv_input3_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::ap_uv_output), aid_cast (action_test::ap_uv_output_parameter));
      
  //     binding_manager_->bind<action_test::up_v_output_traits,
  // 	action_test::p_v_input1_traits> (output_automaton, &action_test::up_v_output,
  // 					 input_automaton, &action_test::p_v_input1, action_test::p_v_input1_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::up_v_output), 0);
      
  //     binding_manager_->bind<action_test::p_v_output_traits,
  //     	action_test::p_v_input2_traits> (output_automaton, &action_test::p_v_output, action_test::p_v_output_parameter,
  // 					 input_automaton, &action_test::p_v_input2, action_test::p_v_input2_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::p_v_output), aid_cast (action_test::p_v_output_parameter));
      
  //     action_test::ap_v_output_parameter = input_automaton->aid ();
  //     binding_manager_->bind<action_test::ap_v_output_traits,
  //     	action_test::p_v_input3_traits> (output_automaton, &action_test::ap_v_output, action_test::ap_v_output_parameter,
  // 					 input_automaton, &action_test::p_v_input3, action_test::p_v_input3_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::ap_v_output), aid_cast (action_test::ap_v_output_parameter));

  //     action_test::ap_uv_input1_parameter = output_automaton->aid ();
  //     binding_manager_->bind<action_test::up_uv_output_traits,
  // 	action_test::ap_uv_input1_traits> (output_automaton, &action_test::up_uv_output,
  // 					   input_automaton, &action_test::ap_uv_input1, action_test::ap_uv_input1_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::up_uv_output), 0);

  //     action_test::ap_uv_input2_parameter = output_automaton->aid ();
  //     binding_manager_->bind<action_test::p_uv_output_traits,
  // 	action_test::ap_uv_input2_traits> (output_automaton, &action_test::p_uv_output, action_test::p_uv_output_parameter,
  // 					   input_automaton, &action_test::ap_uv_input2, action_test::ap_uv_input2_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::p_uv_output), aid_cast (action_test::p_uv_output_parameter));
      
  //     action_test::ap_uv_input3_parameter = output_automaton->aid ();
  //     action_test::ap_uv_output_parameter = input_automaton->aid ();
  //     binding_manager_->bind<action_test::ap_uv_output_traits,
  // 	action_test::ap_uv_input3_traits> (output_automaton, &action_test::ap_uv_output, action_test::ap_uv_output_parameter,
  // 					   input_automaton, &action_test::ap_uv_input3, action_test::ap_uv_input3_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::ap_uv_output), aid_cast (action_test::ap_uv_output_parameter));
      
  //     action_test::ap_v_input1_parameter = output_automaton->aid ();
  //     binding_manager_->bind<action_test::up_v_output_traits,
  // 	action_test::ap_v_input1_traits> (output_automaton, &action_test::up_v_output,
  // 					  input_automaton, &action_test::ap_v_input1, action_test::ap_v_input1_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::up_v_output), 0);
      
  //     action_test::ap_v_input2_parameter = output_automaton->aid ();
  //     binding_manager_->bind<action_test::p_v_output_traits,
  // 	action_test::ap_v_input2_traits> (output_automaton, &action_test::p_v_output, action_test::p_v_output_parameter,
  // 					  input_automaton, &action_test::ap_v_input2, action_test::ap_v_input2_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::p_v_output), aid_cast (action_test::p_v_output_parameter));
      
  //     action_test::ap_v_input3_parameter = output_automaton->aid ();
  //     action_test::ap_v_output_parameter = input_automaton->aid ();
  //     binding_manager_->bind<action_test::ap_v_output_traits,
  // 	action_test::ap_v_input3_traits> (output_automaton, &action_test::ap_v_output, action_test::ap_v_output_parameter,
  // 					  input_automaton, &action_test::ap_v_input3, action_test::ap_v_input3_parameter);
  //     system_scheduler_->schedule_action (output_automaton, reinterpret_cast<const void*> (&action_test::ap_v_output), aid_cast (action_test::ap_v_output_parameter));

  //   }
  }

  void
  run ()
  {
    bool r;
    vm_area_interface* area;

    // Allocate the system automaton.
    automaton* sa = new (system_alloc ()) automaton (aid_manager_.allocate (), descriptor_constants::RING0, vm_manager::page_directory_physical_address ());

    // Build its memory map and mark the frames as already being used.

    // Low memory.
    // The page directory uses the same page table for high and low memory.
    // We only mark the high memory.
    area = new (system_alloc ()) vm_reserved_area (KERNEL_VIRTUAL_BASE, reinterpret_cast<const uint8_t*> (KERNEL_VIRTUAL_BASE) + ONE_MEGABYTE);
    r = sa->insert_vm_area (area);
    kassert (r);

    // Text.
    area = new (system_alloc ()) vm_text_area (&text_begin, &text_end);
    r = sa->insert_vm_area (area);
    kassert (r);

    // Read-only data.
    area = new (system_alloc ()) vm_rodata_area (&rodata_begin, &rodata_end);
    r = sa->insert_vm_area (area);
    kassert (r);

    // Data.
    area = new (system_alloc ()) vm_data_area (&data_begin, &data_end);
    r = sa->insert_vm_area (area);
    kassert (r);

    // Stack.
    vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_AUTOMATON_STACK_BEGIN,
								     SYSTEM_AUTOMATON_STACK_END);
    r = sa->insert_stack_area (stack_area);
    kassert (r);

    // Heap.
    vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (system_alloc::heap_begin ());
    r = sa->insert_heap_area (heap_area);
    kassert (r);
    // Tell the heap about the existing heap.
    // (We can't allocate any more memory until the page tables are corrected.)
    void* hb = sa->allocate (system_alloc::heap_size ());
    kassert (hb == system_alloc::heap_begin ());

    // Mark frames that are currently being used and update the logical address space.
    page_directory* pd = vm_manager::get_page_directory ();
    for (size_t i = 0; i < PAGE_ENTRY_COUNT; ++i) {
      if (pd->entry[i].present_) {
	page_table* pt = vm_manager::get_page_table (reinterpret_cast<const void*> (i << 22));
	for (size_t j = 0; j < PAGE_ENTRY_COUNT; ++j) {
	  if (pt->entry[j].present_) {
	    const void* address = get_address (i, j);
	    // The page directory uses the same page table for low and high memory.  Only process high memory.
	    if (address >= KERNEL_VIRTUAL_BASE) {
	      if (sa->address_in_use (address)) {
		// If the address is in the logical address space, mark the frame as being used.
		frame_manager::mark_as_used (pt->entry[j].frame_);
	      }
	      else {
		// Otherwise, unmap it.
		vm_manager::unmap (address);
	      }
	    }
	  }
	}
      }
    }

    // Map in the kernel page directory to a fixed location.
    // When we copy page directories, the kernel directory will always be accessible.
    vm_manager::map (vm_manager::get_kernel_page_directory (), physical_address_to_frame (vm_manager::page_directory_physical_address ()), paging_constants::SUPERVISOR, paging_constants::WRITABLE);

    // This line instructs the system allocator to start using the system automaton's memory map.
    system_automaton = sa;

    /* Add the actions. */
    system_automaton->add_action<init_traits> (&init);

    // Add the system automaton to the system scheduler.
    system_scheduler_.add_automaton (system_automaton);
    
    create_action_test ();
    // //create_ramdisk ();

    // Start the scheduler.  Doesn't return.
    system_scheduler_.finish_action (0, 0, false, 0);
  }

  void
  page_fault (const void* address,
	      uint32_t error,
	      registers* regs)
  {
    kassert (0);
    // automaton_type* current_automaton = system_scheduler_->current_automaton ();
    // kassert (current_automaton != system_automaton_);

    // if (address < KERNEL_VIRTUAL_BASE) {
    //   // Use the automaton's memory map.
    //   current_automaton->page_fault (address, error, regs);
    // }
    // else {
    //   // Use our memory map.
    //   system_automaton_->page_fault (address, error, regs);
    // }
  }

  void
  finish_action (const void* action_entry_point,
		 aid_t parameter,
		 bool output_status,
		 const void* buffer)
  {
    kassert (0);
    //system_scheduler_->finish_action (action_entry_point, parameter, output_status, buffer);
  }

  void*
  alloc (size_t size)
  {
    kassert (0);
    // automaton_type* current_automaton = system_scheduler_->current_automaton ();
    // kassert (current_automaton != system_automaton_);

    // return current_automaton->alloc (size);
    return 0;
  }

  void
  unknown_system_call (void)
  {
    // TODO
    kassert (0);
  }
  
  void
  bad_schedule (void)
  {
    // TODO
    kassert (0);
  }

  void
  bad_value (void)
  {
    // TODO
    kassert (0);
  }

}
