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
//#include "binding_manager.hpp"
//#include "automaton.hpp"
#include "fifo_scheduler.hpp"
// #include "action_test_automaton.hpp"
// #include "ramdisk_automaton.hpp"
#include "kassert.hpp"

// Symbols to build the kernel's memory map.
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

namespace system_automaton {

  /* Size of the stack used for "system" automata.
     Must be large enough for functions and interrupts. */
  static const size_t SYSTEM_STACK_SIZE = 0x1000;
  
  static scheduler system_scheduler_;
  static aid_manager aid_manager_;
  automaton* system_automaton = 0;

  // typedef fifo_scheduler<system_alloc, system_allocator> scheduler_type;
  // static scheduler_type* scheduler_;

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

  // static void
  // create_action_test ()
  // {
  //   /* Create automata to test actions. */
  //   automaton_type* input_automaton;
  //   automaton_type* output_automaton;

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
  //     input_automaton = new (alloc_) automaton_type (alloc_, aid_manager_->allocate (), descriptor_constants::RING0, frame_to_physical_address (frame), vm_manager::page_directory_logical_address (), vm_manager::page_directory_logical_address (), paging_constants::SUPERVISOR);
      
  //     /* Third, create the automaton's memory map. */
  //     {
  // 	//Reserve low memory.
  // 	bool r = input_automaton->insert_vm_area (new (alloc_) vm_reserved_area (0, ONE_MEGABYTE));
  // 	kassert (r);
	
  // 	// Program text.
  // 	r = input_automaton->insert_vm_area (new (alloc_) vm_text_area (&text_begin, &text_end, paging_constants::SUPERVISOR));
  // 	kassert (r);

  // 	// Program read-only data.
  // 	r = input_automaton->insert_vm_area (new (alloc_) vm_rodata_area (&rodata_begin, &rodata_end, paging_constants::SUPERVISOR));
  // 	kassert (r);

  // 	// Program data.
  // 	r = input_automaton->insert_vm_area (new (alloc_) vm_data_area (&data_begin, &data_end, paging_constants::SUPERVISOR));
  // 	kassert (r);
	
  // 	// TODO: Reserve additional memory to prevent corrupt of the heap.
	
  // 	/* Add a stack area. */
  // 	vm_stack_area* stack = new (alloc_) vm_stack_area (static_cast<const uint8_t*> (input_automaton->stack_pointer ()) - SYSTEM_STACK_SIZE,
  // 							   input_automaton->stack_pointer (),
  // 							   paging_constants::SUPERVISOR);
  // 	r = input_automaton->insert_vm_area (stack);
  // 	kassert (r);
  // 	/* Back with physical pages.  See previous comment about the stack. */
  // 	stack->back_with_frames ();
  //     }
      
  //     /* Fourth, add the actions. */
  //     input_automaton->add_action<action_test::up_uv_input1_traits> (&action_test::up_uv_input1);
  //     input_automaton->add_action<action_test::up_uv_input2_traits> (&action_test::up_uv_input2);
  //     input_automaton->add_action<action_test::up_uv_input3_traits> (&action_test::up_uv_input3);
  //     input_automaton->add_action<action_test::up_v_input1_traits> (&action_test::up_v_input1);
  //     input_automaton->add_action<action_test::up_v_input2_traits> (&action_test::up_v_input2);
  //     input_automaton->add_action<action_test::up_v_input3_traits> (&action_test::up_v_input3);
  //     input_automaton->add_action<action_test::p_uv_input1_traits> (&action_test::p_uv_input1);
  //     input_automaton->add_action<action_test::p_uv_input2_traits> (&action_test::p_uv_input2);
  //     input_automaton->add_action<action_test::p_uv_input3_traits> (&action_test::p_uv_input3);
  //     input_automaton->add_action<action_test::p_v_input1_traits> (&action_test::p_v_input1);
  //     input_automaton->add_action<action_test::p_v_input2_traits> (&action_test::p_v_input2);
  //     input_automaton->add_action<action_test::p_v_input3_traits> (&action_test::p_v_input3);
  //     input_automaton->add_action<action_test::ap_uv_input1_traits> (&action_test::ap_uv_input1);
  //     input_automaton->add_action<action_test::ap_uv_input2_traits> (&action_test::ap_uv_input2);
  //     input_automaton->add_action<action_test::ap_uv_input3_traits> (&action_test::ap_uv_input3);
  //     input_automaton->add_action<action_test::ap_v_input1_traits> (&action_test::ap_v_input1);
  //     input_automaton->add_action<action_test::ap_v_input2_traits> (&action_test::ap_v_input2);
  //     input_automaton->add_action<action_test::ap_v_input3_traits> (&action_test::ap_v_input3);

  //     /* Fifth, add the automaton to the system scheduler. */
  //     system_scheduler_->add_automaton (input_automaton);
      
  //     /* Sixth, bind. */
  //   }

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
  // }

  static void
  create_ramdisk ()
  {
    // /* Create the ramdisk automaton. */
    
    // /* First, create a new page directory. */
    
    // /* Reserve some logical address space for the page directory. */
    // page_directory* pd = static_cast<page_directory*> (system_automaton_->reserve (PAGE_SIZE));
    // kassert (pd != 0);
    // // Allocate a frame.
    // frame_t frame = frame_manager::alloc ();
    // /* Map the page directory. */
    // vm_manager::map (pd, frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE);
    // /* Initialize the page directory with a copy of the current page directory. */
    // pd->initialize_with_current (frame_to_physical_address (frame));
    // /* Unmap. */
    // vm_manager::unmap (pd);
    // /* Unreserve. */
    // system_automaton_->unreserve (pd);
    
    // /* Switch to the new page directory. */
    // vm_manager::switch_to_directory (frame_to_physical_address (frame));
    
    // /* Second, create the automaton. */
    // automaton_type* ramdisk = new (alloc_) automaton_type (alloc_, descriptor_constants::RING0, frame_to_physical_address (frame), KERNEL_VIRTUAL_BASE, KERNEL_VIRTUAL_BASE, paging_constants::SUPERVISOR);

    // /* Third, create the automaton's memory map. */
    // {
    //   // Reserve low memory.
    //   kassert (ramdisk->insert_vm_area (new (alloc_) vm_reserved_area (0, ONE_MEGABYTE)));
      
    //   /* Add a stack area. */
    //   vm_stack_area* stack = new (alloc_) vm_stack_area (static_cast<const uint8_t*> (ramdisk->stack_pointer ()) - SYSTEM_STACK_SIZE,
    // 							 ramdisk->stack_pointer (),
    // 							 paging_constants::SUPERVISOR);
    //   kassert (ramdisk->insert_vm_area (stack));
    //   /* Back with physical pages.  See previous comment about the stack. */
    //   stack->back_with_frames ();
    // }
    
    // /* Fourth, add the actions. */
    // ramdisk->add_action<ramdisk::init_traits> ();
    // ramdisk->add_action<ramdisk::read_request_traits> ();

    // /* Fifth, bind. */
    // binding_manager_->bind<init_traits, ramdisk::init_traits> (system_automaton_, ramdisk,
    // 							       ramdisk, 0);

    // /* Sixth, add the automaton to the system scheduler. */
    // system_scheduler_->add_automaton (ramdisk);

    // /* Seventh, initialize the new automaton. */
    // init_queue_->push_back (ramdisk);
  }

// void
// vm_manager::initialize (const void* placement_begin,
// 			const void* placement_end)
// {
//   physical_address_t low_page_table_paddr = logical_to_physical (&low_page_table, KERNEL_VIRTUAL_BASE);
//   physical_address_t kernel_page_directory_paddr = logical_to_physical (&kernel_page_directory, KERNEL_VIRTUAL_BASE);

//   /* Clear the page directory and page table. */
//   low_page_table.clear ();
//   kernel_page_directory.clear (kernel_page_directory_paddr);

//   /* Insert the page table. */
//   kernel_page_directory.entry[get_page_directory_entry (0)] = page_directory_entry (physical_address_to_frame (low_page_table_paddr), paging_constants::PRESENT);
//   kernel_page_directory.entry[get_page_directory_entry (KERNEL_VIRTUAL_BASE)] = page_directory_entry (physical_address_to_frame (low_page_table_paddr), paging_constants::PRESENT);

//   const void* begin;
//   const void* end;

//   /* Identity map the first 1MB. */
//   begin = 0;
//   end = ONE_MEGABYTE;
//   kassert (end < INITIAL_LOGICAL_LIMIT);
//   for (; begin < end; begin = static_cast<const uint8_t*> (begin) + PAGE_SIZE) {
//     const frame_t frame = physical_address_to_frame (logical_to_physical (begin, 0));
//     low_page_table.entry[get_page_table_entry (begin)] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE, paging_constants::PRESENT);
//     frame_manager::mark_as_used (frame);
//   }

//   /* Map the kernel text. */
//   begin = align_down (&text_begin, PAGE_SIZE);
//   end = align_up (&text_end, PAGE_SIZE);
//   kassert (end < INITIAL_LOGICAL_LIMIT);
//   for (; begin < end; begin = static_cast<const uint8_t*> (begin) + PAGE_SIZE) {
//     const frame_t frame = physical_address_to_frame (logical_to_physical (begin, KERNEL_VIRTUAL_BASE));
//     low_page_table.entry[get_page_table_entry (begin)] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::PRESENT);
//     frame_manager::mark_as_used (frame);
//   }

//   /* Map the kernel read-only data. */
//   begin = align_down (&rodata_begin, PAGE_SIZE);
//   end = align_up (&rodata_end, PAGE_SIZE);
//   kassert (end < INITIAL_LOGICAL_LIMIT);
//   for (; begin < end; begin = static_cast<const uint8_t*> (begin) + PAGE_SIZE) {
//     const frame_t frame = physical_address_to_frame (logical_to_physical (begin, KERNEL_VIRTUAL_BASE));
//     low_page_table.entry[get_page_table_entry (begin)] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::PRESENT);
//     frame_manager::mark_as_used (frame);
//   }

//   /* Map the kernel data. */
//   begin = align_down (&data_begin, PAGE_SIZE);
//   end = align_up (&data_end, PAGE_SIZE);
//   kassert (end < INITIAL_LOGICAL_LIMIT);
//   for (; begin < end; begin = static_cast<const uint8_t*> (begin) + PAGE_SIZE) {
//     const frame_t frame = physical_address_to_frame (logical_to_physical (begin, KERNEL_VIRTUAL_BASE));
//     low_page_table.entry[get_page_table_entry (begin)] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE, paging_constants::PRESENT);
//     frame_manager::mark_as_used (frame);
//   }

//   /* Map the memory allocated by placement. */
//   begin = align_down (placement_begin, PAGE_SIZE);
//   end = align_up (placement_end, PAGE_SIZE);
//   kassert (end < INITIAL_LOGICAL_LIMIT);
//   for (; begin < end; begin = static_cast<const uint8_t*> (begin) + PAGE_SIZE) {
//     const frame_t frame = physical_address_to_frame (logical_to_physical (begin, KERNEL_VIRTUAL_BASE));
//     low_page_table.entry[get_page_table_entry (begin)] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE, paging_constants::PRESENT);
//     frame_manager::mark_as_used (frame);
//   }

//   switch_to_directory (kernel_page_directory_paddr);
// }
    
  void
  run ()
  {
    bool r;
    const uint8_t* ptr;
    vm_area_interface* area;

    // Allocate the system automaton.
    automaton* sa = new (system_alloc ()) automaton (aid_manager_.allocate (), descriptor_constants::RING0);

    // Build its memory map and mark the frames as already being used.

    // Low memory.
    area = new (system_alloc ()) vm_reserved_area (0, reinterpret_cast<const void*> (ONE_MEGABYTE));
    r = sa->insert_vm_area (area);
    kassert (r);

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
    area = new (system_alloc ()) vm_stack_area (reinterpret_cast<uint8_t*> (vm_manager::get_page_directory ()) - SYSTEM_STACK_SIZE,
						vm_manager::get_page_directory ());
    r = sa->insert_vm_area (area);
    kassert (r);

    // Heap.
    vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (system_alloc::heap_begin ());
    r = sa->insert_vm_area (heap_area);
    kassert (r);
    // Tell the heap about the existing heap.
    heap_area->allocate (system_alloc::heap_size ());

    // We can't allocate any more memory until the page tables are correct.

    // Mark frames that are currently being used and update the logical address space.
    page_directory* pd = vm_manager::get_page_directory ();
    for (size_t i = 0; i < 1024; ++i) {
      if (pd->entry[i].present_) {
	page_table* pt = vm_manager::get_page_table (reinterpret_cast<const void*> (i << 22));
	for (size_t j = 0; j < 1024; ++j) {
	  if (pt->entry[j].present_) {
	    const void* address = reinterpret_cast<const void*> (i << 22 | j << 12);
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

    // This line instructs the system allocator to start using the system automaton's memory map.
    system_automaton = sa;

    /* Add the actions. */
    system_automaton->add_action<init_traits> (&init);

    kassert (0);
    
    // // Add the system automaton to the system scheduler.
    // system_scheduler_->add_automaton (system_automaton_);
    
    // // Allocate the data structures for the system automaton.
    // scheduler_ = new (alloc_) scheduler_type (alloc_);

    // create_action_test ();
    // //create_ramdisk ();
    
    // // Start the scheduler.  Doesn't return.
    // system_scheduler_->finish_action (0, 0, false, 0);
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
