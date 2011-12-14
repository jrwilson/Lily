/*
  File
  ----
  rts.cpp
  
  Description
  -----------
  The run-time system (rts).
  Contains the system automaton, the set of bindings, and the scheduler.

  Authors:
  Justin R. Wilson
*/

#include "rts.hpp"
#include "system_automaton.hpp"
#include "action_test_automaton.hpp"

// Symbols to build the kernel's memory map.
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

// Stack for the system automaton.
static const void* const SYSTEM_AUTOMATON_STACK_BEGIN = reinterpret_cast<const uint8_t*> (PAGING_AREA) - 0x1000;
static const void* const SYSTEM_AUTOMATON_STACK_END = PAGING_AREA;

// Heap location for system automata.
// Starting at 4MB allow us to use the low page table of the kernel.
static const void* const SYSTEM_HEAP_BEGIN = reinterpret_cast<const void*> (FOUR_MEGABYTES);

// Stack range for system automata.
static const void* const SYSTEM_STACK_BEGIN = reinterpret_cast<const uint8_t*> (KERNEL_VIRTUAL_BASE) - 0x1000;
static const void* const SYSTEM_STACK_END = KERNEL_VIRTUAL_BASE;

void
rts::run ()
{
  bool r;
  vm_area_interface* area;
  
  // Allocate the system automaton.
  automaton* sa = new (system_alloc ()) automaton (descriptor_constants::RING0, vm::page_directory_frame ());
  
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
								   SYSTEM_AUTOMATON_STACK_END,
								   vm::SUPERVISOR);
  r = sa->insert_stack_area (stack_area);
  kassert (r);
  
  // Heap.
  vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (system_alloc::heap_begin (), vm::SUPERVISOR);
  r = sa->insert_heap_area (heap_area);
  kassert (r);
  // Tell the heap about the existing heap.
  void* hb = sa->sbrk (system_alloc::heap_size ());
  kassert (hb == system_alloc::heap_begin ());
  
  // This line instructs the system allocator to start using the system automaton's memory map.
  // It also makes it available for initializing virtual memory.
  system_automaton = sa;
  
  // We can't allocate any more memory until the page tables are corrected.
  vm::initialize (system_automaton);
  
  /* Add the actions. */
  system_automaton->add_action<system_automaton::init_traits> (&system_automaton::init);
  
  // Add the system automaton to the system scheduler.
  scheduler_.add_automaton (system_automaton);
  
  create_action_test ();
  
  // Start the scheduler.  Doesn't return.
  scheduler_.finish (false, 0);
}

rts::scheduler_type rts::scheduler_;

void
rts::create_action_test ()
{
  // Create automata to test actions.
  automaton* input_automaton;
  automaton* output_automaton;

  {    
    // First, create a new page directory.

    // Reserve some logical address space for the page directory.
    vm::page_directory* pd = static_cast<vm::page_directory*> (vm::get_stub ());
    // Allocate a frame.
    frame_t frame = frame_manager::alloc ();
    // Map the page directory.
    vm::map (pd, frame, vm::SUPERVISOR, vm::WRITABLE);
    // Initialize the page directory with a copy of the current page directory.
    new (pd) vm::page_directory (true);
    // Unmap.
    vm::unmap (pd);
    // Drop the reference count from allocation.
    size_t count = frame_manager::decref (frame);
    kassert (count == 1);

    // Second, create the automaton.
    input_automaton = new (system_alloc ()) automaton (descriptor_constants::RING0, frame);

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
      vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN, vm::SUPERVISOR);
      r = input_automaton->insert_heap_area (heap_area);
      kassert (r);

      // Stack.
      vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END, vm::SUPERVISOR);
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
    scheduler_.add_automaton (input_automaton);
      
    // Sixth, bind.
  }

  {
    // First, create a new page directory.

    // Reserve some logical address space for the page directory.
    vm::page_directory* pd = static_cast<vm::page_directory*> (vm::get_stub ());
    // Allocate a frame.
    frame_t frame = frame_manager::alloc ();
    // Map the page directory.
    vm::map (pd, frame, vm::SUPERVISOR, vm::WRITABLE);
    // Initialize the page directory with a copy of the current page directory.
    new (pd) vm::page_directory (true);
    // Unmap.
    vm::unmap (pd);
    // Drop the reference count from allocation.
    size_t count = frame_manager::decref (frame);
    kassert (count == 1);

    // Second, create the automaton.
    output_automaton = new (system_alloc ()) automaton (descriptor_constants::RING0, frame);
      
    // Third, create the automaton's memory map.
    {
      vm_area_interface* area;
      bool r;

      // Text.
      area = new (system_alloc ()) vm_text_area (&text_begin, &text_end);
      r = output_automaton->insert_vm_area (area);
      kassert (r);
	
      // Read-only data.
      area = new (system_alloc ()) vm_rodata_area (&rodata_begin, &rodata_end);
      r = output_automaton->insert_vm_area (area);
      kassert (r);
	
      // Data.
      area = new (system_alloc ()) vm_data_area (&data_begin, &data_end);
      r = output_automaton->insert_vm_area (area);
      kassert (r);

      // Heap.
      vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN, vm::SUPERVISOR);
      r = output_automaton->insert_heap_area (heap_area);
      kassert (r);

      // Stack.
      vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END, vm::SUPERVISOR);
      r = output_automaton->insert_stack_area (stack_area);
      kassert (r);
    }
      
    // Fourth, add the actions.
    output_automaton->add_action<action_test::init_traits> (&action_test::init);
    output_automaton->add_action<action_test::up_uv_output_traits> (&action_test::up_uv_output);
    output_automaton->add_action<action_test::up_v_output_traits> (&action_test::up_v_output);
    output_automaton->add_action<action_test::p_uv_output_traits> (&action_test::p_uv_output);
    output_automaton->add_action<action_test::p_v_output_traits> (&action_test::p_v_output);
    output_automaton->add_action<action_test::ap_uv_output_traits> (&action_test::ap_uv_output);
    output_automaton->add_action<action_test::ap_v_output_traits> (&action_test::ap_v_output);

    output_automaton->add_action<action_test::up_internal_traits> (&action_test::up_internal);
    output_automaton->add_action<action_test::p_internal_traits> (&action_test::p_internal);

    // Fifth, add the automaton to the system scheduler.
    scheduler_.add_automaton (output_automaton);
      
    // Sixth, bind.
    binding_manager::bind<system_automaton::init_traits,
  			  action_test::init_traits> (system_automaton, &system_automaton::init, output_automaton,
  						     output_automaton, &action_test::init);
    checked_schedule (system_automaton, reinterpret_cast<const void*> (&system_automaton::init), aid_cast (output_automaton));
    
    binding_manager::bind<action_test::up_uv_output_traits,
  			  action_test::up_uv_input1_traits> (output_automaton, &action_test::up_uv_output,
  							     input_automaton, &action_test::up_uv_input1);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_uv_output));
    
    binding_manager::bind<action_test::p_uv_output_traits,
  			  action_test::up_uv_input2_traits> (output_automaton, &action_test::p_uv_output, action_test::p_uv_output_parameter,
  							     input_automaton, &action_test::up_uv_input2);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_uv_output), aid_cast (action_test::p_uv_output_parameter));
    
    action_test::ap_uv_output_parameter = input_automaton->aid ();
    binding_manager::bind<action_test::ap_uv_output_traits,
  			  action_test::up_uv_input3_traits> (output_automaton, &action_test::ap_uv_output, action_test::ap_uv_output_parameter,
  							     input_automaton, &action_test::up_uv_input3);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_uv_output), aid_cast (action_test::ap_uv_output_parameter));

    binding_manager::bind<action_test::up_v_output_traits,
  			  action_test::up_v_input1_traits> (output_automaton, &action_test::up_v_output,
  							    input_automaton, &action_test::up_v_input1);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_v_output));
      
    binding_manager::bind<action_test::p_v_output_traits,
  			  action_test::up_v_input2_traits> (output_automaton, &action_test::p_v_output, action_test::p_v_output_parameter,
  							    input_automaton, &action_test::up_v_input2);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_v_output), aid_cast (action_test::p_v_output_parameter));
			 
    action_test::ap_v_output_parameter = input_automaton->aid ();
    binding_manager::bind<action_test::ap_v_output_traits,
  			  action_test::up_v_input3_traits> (output_automaton, &action_test::ap_v_output, action_test::ap_v_output_parameter,
  							    input_automaton, &action_test::up_v_input3);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_v_output), aid_cast (action_test::ap_v_output_parameter));

    binding_manager::bind<action_test::up_uv_output_traits,
  			  action_test::p_uv_input1_traits> (output_automaton, &action_test::up_uv_output,
  							    input_automaton, &action_test::p_uv_input1, action_test::p_uv_input1_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_uv_output));
      
    binding_manager::bind<action_test::p_uv_output_traits,
  			  action_test::p_uv_input2_traits> (output_automaton, &action_test::p_uv_output, action_test::p_uv_output_parameter,
  							    input_automaton, &action_test::p_uv_input2, action_test::p_uv_input2_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_uv_output), aid_cast (action_test::p_uv_output_parameter));
      
    action_test::ap_uv_output_parameter = input_automaton->aid ();
    binding_manager::bind<action_test::ap_uv_output_traits,
  			  action_test::p_uv_input3_traits> (output_automaton, &action_test::ap_uv_output, action_test::ap_uv_output_parameter,
  							    input_automaton, &action_test::p_uv_input3, action_test::p_uv_input3_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_uv_output), aid_cast (action_test::ap_uv_output_parameter));
      
    binding_manager::bind<action_test::up_v_output_traits,
  			  action_test::p_v_input1_traits> (output_automaton, &action_test::up_v_output,
  							   input_automaton, &action_test::p_v_input1, action_test::p_v_input1_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_v_output));
      
    binding_manager::bind<action_test::p_v_output_traits,
  			  action_test::p_v_input2_traits> (output_automaton, &action_test::p_v_output, action_test::p_v_output_parameter,
  							   input_automaton, &action_test::p_v_input2, action_test::p_v_input2_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_v_output), aid_cast (action_test::p_v_output_parameter));
      
    action_test::ap_v_output_parameter = input_automaton->aid ();
    binding_manager::bind<action_test::ap_v_output_traits,
  			  action_test::p_v_input3_traits> (output_automaton, &action_test::ap_v_output, action_test::ap_v_output_parameter,
  							   input_automaton, &action_test::p_v_input3, action_test::p_v_input3_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_v_output), aid_cast (action_test::ap_v_output_parameter));

    action_test::ap_uv_input1_parameter = output_automaton->aid ();
    binding_manager::bind<action_test::up_uv_output_traits,
  			  action_test::ap_uv_input1_traits> (output_automaton, &action_test::up_uv_output,
  							     input_automaton, &action_test::ap_uv_input1, action_test::ap_uv_input1_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_uv_output));

    action_test::ap_uv_input2_parameter = output_automaton->aid ();
    binding_manager::bind<action_test::p_uv_output_traits,
  			  action_test::ap_uv_input2_traits> (output_automaton, &action_test::p_uv_output, action_test::p_uv_output_parameter,
  							     input_automaton, &action_test::ap_uv_input2, action_test::ap_uv_input2_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_uv_output), aid_cast (action_test::p_uv_output_parameter));
      
    action_test::ap_uv_input3_parameter = output_automaton->aid ();
    action_test::ap_uv_output_parameter = input_automaton->aid ();
    binding_manager::bind<action_test::ap_uv_output_traits,
  			  action_test::ap_uv_input3_traits> (output_automaton, &action_test::ap_uv_output, action_test::ap_uv_output_parameter,
  							     input_automaton, &action_test::ap_uv_input3, action_test::ap_uv_input3_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_uv_output), aid_cast (action_test::ap_uv_output_parameter));
      
    action_test::ap_v_input1_parameter = output_automaton->aid ();
    binding_manager::bind<action_test::up_v_output_traits,
  			  action_test::ap_v_input1_traits> (output_automaton, &action_test::up_v_output,
  							    input_automaton, &action_test::ap_v_input1, action_test::ap_v_input1_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_v_output));
      
    action_test::ap_v_input2_parameter = output_automaton->aid ();
    binding_manager::bind<action_test::p_v_output_traits,
  			  action_test::ap_v_input2_traits> (output_automaton, &action_test::p_v_output, action_test::p_v_output_parameter,
  							    input_automaton, &action_test::ap_v_input2, action_test::ap_v_input2_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_v_output), aid_cast (action_test::p_v_output_parameter));
      
    action_test::ap_v_input3_parameter = output_automaton->aid ();
    action_test::ap_v_output_parameter = input_automaton->aid ();
    binding_manager::bind<action_test::ap_v_output_traits,
  			  action_test::ap_v_input3_traits> (output_automaton, &action_test::ap_v_output, action_test::ap_v_output_parameter,
  							    input_automaton, &action_test::ap_v_input3, action_test::ap_v_input3_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_v_output), aid_cast (action_test::ap_v_output_parameter));
  }
}
