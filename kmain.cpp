/*
  File
  ----
  kmain.cpp
  
  Description
  -----------
  Main function of the kernel.

  Authors
  -------
  http://wiki.osdev.org/Bare_bones
  Justin R. Wilson
*/

#include "kout.hpp"
#include "multiboot_parser.hpp"
#include "system_allocator.hpp"
#include "vm_def.hpp"
#include "gdt.hpp"
#include "idt.hpp"
#include "exception_handler.hpp"
#include "irq_handler.hpp"
#include "trap_handler.hpp"
#include "frame_manager.hpp"
#include "rts.hpp"
#include "kassert.hpp"

#include "scheduler.hpp"
#include "system_automaton.hpp"
#include "action_test_automaton.hpp"
#include "buffer_test_automaton.hpp"

// Stack for the system automaton.
static const void* const SYSTEM_AUTOMATON_STACK_BEGIN = reinterpret_cast<const uint8_t*> (PAGING_AREA) - 0x1000;
static const void* const SYSTEM_AUTOMATON_STACK_END = PAGING_AREA;

// Heap location for system automata.
// Starting at 4MB allow us to use the low page table of the kernel.
static const void* const SYSTEM_HEAP_BEGIN = reinterpret_cast<const void*> (FOUR_MEGABYTES);

// Stack range for system automata.
static const void* const SYSTEM_STACK_BEGIN = reinterpret_cast<const uint8_t*> (KERNEL_VIRTUAL_BASE) - 0x1000;
static const void* const SYSTEM_STACK_END = KERNEL_VIRTUAL_BASE;

// Symbols to build the kernel's memory map.
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

extern int* ctors_begin;
extern int* ctors_end;

static void
checked_schedule (automaton* a,
		  const void* aep,
		  aid_t p = 0)
{
  automaton::const_action_iterator pos = a->action_find (aep);
  kassert (pos != a->action_end ());
  scheduler::schedule (caction (a, *pos, p));
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
    input_automaton = rts::create (descriptor_constants::RING0, frame);

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
    output_automaton = rts::create (descriptor_constants::RING0, frame);
      
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

    // Bind.
    rts::bind<system_automaton::init_traits,
	      action_test::init_traits> (system_automaton::system_automaton, &system_automaton::init, output_automaton,
					 output_automaton, &action_test::init);
    checked_schedule (system_automaton::system_automaton, reinterpret_cast<const void*> (&system_automaton::init), aid_cast (output_automaton));
    
    rts::bind<action_test::up_uv_output_traits,
	      action_test::up_uv_input1_traits> (output_automaton, &action_test::up_uv_output,
						 input_automaton, &action_test::up_uv_input1);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_uv_output));
    
    rts::bind<action_test::p_uv_output_traits,
	      action_test::up_uv_input2_traits> (output_automaton, &action_test::p_uv_output, action_test::p_uv_output_parameter,
						 input_automaton, &action_test::up_uv_input2);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_uv_output), aid_cast (action_test::p_uv_output_parameter));
    
    action_test::ap_uv_output_parameter = input_automaton->aid ();
    rts::bind<action_test::ap_uv_output_traits,
	      action_test::up_uv_input3_traits> (output_automaton, &action_test::ap_uv_output, action_test::ap_uv_output_parameter,
						 input_automaton, &action_test::up_uv_input3);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_uv_output), aid_cast (action_test::ap_uv_output_parameter));
    
    rts::bind<action_test::up_v_output_traits,
	      action_test::up_v_input1_traits> (output_automaton, &action_test::up_v_output,
						input_automaton, &action_test::up_v_input1);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_v_output));
    
    rts::bind<action_test::p_v_output_traits,
	      action_test::up_v_input2_traits> (output_automaton, &action_test::p_v_output, action_test::p_v_output_parameter,
						input_automaton, &action_test::up_v_input2);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_v_output), aid_cast (action_test::p_v_output_parameter));
    
    action_test::ap_v_output_parameter = input_automaton->aid ();
    rts::bind<action_test::ap_v_output_traits,
	      action_test::up_v_input3_traits> (output_automaton, &action_test::ap_v_output, action_test::ap_v_output_parameter,
						input_automaton, &action_test::up_v_input3);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_v_output), aid_cast (action_test::ap_v_output_parameter));
    
    rts::bind<action_test::up_uv_output_traits,
	      action_test::p_uv_input1_traits> (output_automaton, &action_test::up_uv_output,
						input_automaton, &action_test::p_uv_input1, action_test::p_uv_input1_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_uv_output));
    
    rts::bind<action_test::p_uv_output_traits,
	      action_test::p_uv_input2_traits> (output_automaton, &action_test::p_uv_output, action_test::p_uv_output_parameter,
						input_automaton, &action_test::p_uv_input2, action_test::p_uv_input2_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_uv_output), aid_cast (action_test::p_uv_output_parameter));
    
    action_test::ap_uv_output_parameter = input_automaton->aid ();
    rts::bind<action_test::ap_uv_output_traits,
	      action_test::p_uv_input3_traits> (output_automaton, &action_test::ap_uv_output, action_test::ap_uv_output_parameter,
						input_automaton, &action_test::p_uv_input3, action_test::p_uv_input3_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_uv_output), aid_cast (action_test::ap_uv_output_parameter));
    
    rts::bind<action_test::up_v_output_traits,
	      action_test::p_v_input1_traits> (output_automaton, &action_test::up_v_output,
					       input_automaton, &action_test::p_v_input1, action_test::p_v_input1_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_v_output));
    
    rts::bind<action_test::p_v_output_traits,
	      action_test::p_v_input2_traits> (output_automaton, &action_test::p_v_output, action_test::p_v_output_parameter,
					       input_automaton, &action_test::p_v_input2, action_test::p_v_input2_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_v_output), aid_cast (action_test::p_v_output_parameter));
    
    action_test::ap_v_output_parameter = input_automaton->aid ();
    rts::bind<action_test::ap_v_output_traits,
	      action_test::p_v_input3_traits> (output_automaton, &action_test::ap_v_output, action_test::ap_v_output_parameter,
					       input_automaton, &action_test::p_v_input3, action_test::p_v_input3_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_v_output), aid_cast (action_test::ap_v_output_parameter));
    
    action_test::ap_uv_input1_parameter = output_automaton->aid ();
    rts::bind<action_test::up_uv_output_traits,
	      action_test::ap_uv_input1_traits> (output_automaton, &action_test::up_uv_output,
						 input_automaton, &action_test::ap_uv_input1, action_test::ap_uv_input1_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_uv_output));
    
    action_test::ap_uv_input2_parameter = output_automaton->aid ();
    rts::bind<action_test::p_uv_output_traits,
	      action_test::ap_uv_input2_traits> (output_automaton, &action_test::p_uv_output, action_test::p_uv_output_parameter,
						 input_automaton, &action_test::ap_uv_input2, action_test::ap_uv_input2_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_uv_output), aid_cast (action_test::p_uv_output_parameter));
    
    action_test::ap_uv_input3_parameter = output_automaton->aid ();
    action_test::ap_uv_output_parameter = input_automaton->aid ();
    rts::bind<action_test::ap_uv_output_traits,
	      action_test::ap_uv_input3_traits> (output_automaton, &action_test::ap_uv_output, action_test::ap_uv_output_parameter,
						 input_automaton, &action_test::ap_uv_input3, action_test::ap_uv_input3_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_uv_output), aid_cast (action_test::ap_uv_output_parameter));
    
    action_test::ap_v_input1_parameter = output_automaton->aid ();
    rts::bind<action_test::up_v_output_traits,
	      action_test::ap_v_input1_traits> (output_automaton, &action_test::up_v_output,
						input_automaton, &action_test::ap_v_input1, action_test::ap_v_input1_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::up_v_output));
    
    action_test::ap_v_input2_parameter = output_automaton->aid ();
    rts::bind<action_test::p_v_output_traits,
	      action_test::ap_v_input2_traits> (output_automaton, &action_test::p_v_output, action_test::p_v_output_parameter,
						input_automaton, &action_test::ap_v_input2, action_test::ap_v_input2_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_v_output), aid_cast (action_test::p_v_output_parameter));
    
    action_test::ap_v_input3_parameter = output_automaton->aid ();
    action_test::ap_v_output_parameter = input_automaton->aid ();
    rts::bind<action_test::ap_v_output_traits,
	      action_test::ap_v_input3_traits> (output_automaton, &action_test::ap_v_output, action_test::ap_v_output_parameter,
						input_automaton, &action_test::ap_v_input3, action_test::ap_v_input3_parameter);
    checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_v_output), aid_cast (action_test::ap_v_output_parameter));
  }
}

static void
create_buffer_test ()
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
    input_automaton = rts::create (descriptor_constants::RING0, frame);

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
    // input_automaton->add_action<buffer_test::up_uv_input1_traits> (&buffer_test::up_uv_input1);
    // input_automaton->add_action<buffer_test::up_uv_input2_traits> (&buffer_test::up_uv_input2);
    // input_automaton->add_action<buffer_test::up_uv_input3_traits> (&buffer_test::up_uv_input3);
    // input_automaton->add_action<buffer_test::up_v_input1_traits> (&buffer_test::up_v_input1);
    // input_automaton->add_action<buffer_test::up_v_input2_traits> (&buffer_test::up_v_input2);
    // input_automaton->add_action<buffer_test::up_v_input3_traits> (&buffer_test::up_v_input3);
    // input_automaton->add_action<buffer_test::p_uv_input1_traits> (&buffer_test::p_uv_input1);
    // input_automaton->add_action<buffer_test::p_uv_input2_traits> (&buffer_test::p_uv_input2);
    // input_automaton->add_action<buffer_test::p_uv_input3_traits> (&buffer_test::p_uv_input3);
    // input_automaton->add_action<buffer_test::p_v_input1_traits> (&buffer_test::p_v_input1);
    // input_automaton->add_action<buffer_test::p_v_input2_traits> (&buffer_test::p_v_input2);
    // input_automaton->add_action<buffer_test::p_v_input3_traits> (&buffer_test::p_v_input3);
    // input_automaton->add_action<buffer_test::ap_uv_input1_traits> (&buffer_test::ap_uv_input1);
    // input_automaton->add_action<buffer_test::ap_uv_input2_traits> (&buffer_test::ap_uv_input2);
    // input_automaton->add_action<buffer_test::ap_uv_input3_traits> (&buffer_test::ap_uv_input3);
    // input_automaton->add_action<buffer_test::ap_v_input1_traits> (&buffer_test::ap_v_input1);
    // input_automaton->add_action<buffer_test::ap_v_input2_traits> (&buffer_test::ap_v_input2);
    // input_automaton->add_action<buffer_test::ap_v_input3_traits> (&buffer_test::ap_v_input3);
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
    output_automaton = rts::create (descriptor_constants::RING0, frame);
      
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
    output_automaton->add_action<buffer_test::init_traits> (&buffer_test::init);
    // output_automaton->add_action<buffer_test::up_uv_output_traits> (&buffer_test::up_uv_output);
    // output_automaton->add_action<buffer_test::up_v_output_traits> (&buffer_test::up_v_output);
    // output_automaton->add_action<buffer_test::p_uv_output_traits> (&buffer_test::p_uv_output);
    // output_automaton->add_action<buffer_test::p_v_output_traits> (&buffer_test::p_v_output);
    // output_automaton->add_action<buffer_test::ap_uv_output_traits> (&buffer_test::ap_uv_output);
    // output_automaton->add_action<buffer_test::ap_v_output_traits> (&buffer_test::ap_v_output);

    // output_automaton->add_action<buffer_test::up_internal_traits> (&buffer_test::up_internal);
    // output_automaton->add_action<buffer_test::p_internal_traits> (&buffer_test::p_internal);

    // Bind.
    rts::bind<system_automaton::init_traits,
	      buffer_test::init_traits> (system_automaton::system_automaton, &system_automaton::init, output_automaton,
					 output_automaton, &buffer_test::init);
    checked_schedule (system_automaton::system_automaton, reinterpret_cast<const void*> (&system_automaton::init), aid_cast (output_automaton));
    
    // rts::bind<buffer_test::up_uv_output_traits,
    // 	      buffer_test::up_uv_input1_traits> (output_automaton, &buffer_test::up_uv_output,
    // 						 input_automaton, &buffer_test::up_uv_input1);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::up_uv_output));
    
    // rts::bind<buffer_test::p_uv_output_traits,
    // 	      buffer_test::up_uv_input2_traits> (output_automaton, &buffer_test::p_uv_output, buffer_test::p_uv_output_parameter,
    // 						 input_automaton, &buffer_test::up_uv_input2);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::p_uv_output), aid_cast (buffer_test::p_uv_output_parameter));
    
    // buffer_test::ap_uv_output_parameter = input_automaton->aid ();
    // rts::bind<buffer_test::ap_uv_output_traits,
    // 	      buffer_test::up_uv_input3_traits> (output_automaton, &buffer_test::ap_uv_output, buffer_test::ap_uv_output_parameter,
    // 						 input_automaton, &buffer_test::up_uv_input3);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::ap_uv_output), aid_cast (buffer_test::ap_uv_output_parameter));
    
    // rts::bind<buffer_test::up_v_output_traits,
    // 	      buffer_test::up_v_input1_traits> (output_automaton, &buffer_test::up_v_output,
    // 						input_automaton, &buffer_test::up_v_input1);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::up_v_output));
    
    // rts::bind<buffer_test::p_v_output_traits,
    // 	      buffer_test::up_v_input2_traits> (output_automaton, &buffer_test::p_v_output, buffer_test::p_v_output_parameter,
    // 						input_automaton, &buffer_test::up_v_input2);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::p_v_output), aid_cast (buffer_test::p_v_output_parameter));
    
    // buffer_test::ap_v_output_parameter = input_automaton->aid ();
    // rts::bind<buffer_test::ap_v_output_traits,
    // 	      buffer_test::up_v_input3_traits> (output_automaton, &buffer_test::ap_v_output, buffer_test::ap_v_output_parameter,
    // 						input_automaton, &buffer_test::up_v_input3);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::ap_v_output), aid_cast (buffer_test::ap_v_output_parameter));
    
    // rts::bind<buffer_test::up_uv_output_traits,
    // 	      buffer_test::p_uv_input1_traits> (output_automaton, &buffer_test::up_uv_output,
    // 						input_automaton, &buffer_test::p_uv_input1, buffer_test::p_uv_input1_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::up_uv_output));
    
    // rts::bind<buffer_test::p_uv_output_traits,
    // 	      buffer_test::p_uv_input2_traits> (output_automaton, &buffer_test::p_uv_output, buffer_test::p_uv_output_parameter,
    // 						input_automaton, &buffer_test::p_uv_input2, buffer_test::p_uv_input2_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::p_uv_output), aid_cast (buffer_test::p_uv_output_parameter));
    
    // buffer_test::ap_uv_output_parameter = input_automaton->aid ();
    // rts::bind<buffer_test::ap_uv_output_traits,
    // 	      buffer_test::p_uv_input3_traits> (output_automaton, &buffer_test::ap_uv_output, buffer_test::ap_uv_output_parameter,
    // 						input_automaton, &buffer_test::p_uv_input3, buffer_test::p_uv_input3_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::ap_uv_output), aid_cast (buffer_test::ap_uv_output_parameter));
    
    // rts::bind<buffer_test::up_v_output_traits,
    // 	      buffer_test::p_v_input1_traits> (output_automaton, &buffer_test::up_v_output,
    // 					       input_automaton, &buffer_test::p_v_input1, buffer_test::p_v_input1_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::up_v_output));
    
    // rts::bind<buffer_test::p_v_output_traits,
    // 	      buffer_test::p_v_input2_traits> (output_automaton, &buffer_test::p_v_output, buffer_test::p_v_output_parameter,
    // 					       input_automaton, &buffer_test::p_v_input2, buffer_test::p_v_input2_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::p_v_output), aid_cast (buffer_test::p_v_output_parameter));
    
    // buffer_test::ap_v_output_parameter = input_automaton->aid ();
    // rts::bind<buffer_test::ap_v_output_traits,
    // 	      buffer_test::p_v_input3_traits> (output_automaton, &buffer_test::ap_v_output, buffer_test::ap_v_output_parameter,
    // 					       input_automaton, &buffer_test::p_v_input3, buffer_test::p_v_input3_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::ap_v_output), aid_cast (buffer_test::ap_v_output_parameter));
    
    // buffer_test::ap_uv_input1_parameter = output_automaton->aid ();
    // rts::bind<buffer_test::up_uv_output_traits,
    // 	      buffer_test::ap_uv_input1_traits> (output_automaton, &buffer_test::up_uv_output,
    // 						 input_automaton, &buffer_test::ap_uv_input1, buffer_test::ap_uv_input1_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::up_uv_output));
    
    // buffer_test::ap_uv_input2_parameter = output_automaton->aid ();
    // rts::bind<buffer_test::p_uv_output_traits,
    // 	      buffer_test::ap_uv_input2_traits> (output_automaton, &buffer_test::p_uv_output, buffer_test::p_uv_output_parameter,
    // 						 input_automaton, &buffer_test::ap_uv_input2, buffer_test::ap_uv_input2_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::p_uv_output), aid_cast (buffer_test::p_uv_output_parameter));
    
    // buffer_test::ap_uv_input3_parameter = output_automaton->aid ();
    // buffer_test::ap_uv_output_parameter = input_automaton->aid ();
    // rts::bind<buffer_test::ap_uv_output_traits,
    // 	      buffer_test::ap_uv_input3_traits> (output_automaton, &buffer_test::ap_uv_output, buffer_test::ap_uv_output_parameter,
    // 						 input_automaton, &buffer_test::ap_uv_input3, buffer_test::ap_uv_input3_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::ap_uv_output), aid_cast (buffer_test::ap_uv_output_parameter));
    
    // buffer_test::ap_v_input1_parameter = output_automaton->aid ();
    // rts::bind<buffer_test::up_v_output_traits,
    // 	      buffer_test::ap_v_input1_traits> (output_automaton, &buffer_test::up_v_output,
    // 						input_automaton, &buffer_test::ap_v_input1, buffer_test::ap_v_input1_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::up_v_output));
    
    // buffer_test::ap_v_input2_parameter = output_automaton->aid ();
    // rts::bind<buffer_test::p_v_output_traits,
    // 	      buffer_test::ap_v_input2_traits> (output_automaton, &buffer_test::p_v_output, buffer_test::p_v_output_parameter,
    // 						input_automaton, &buffer_test::ap_v_input2, buffer_test::ap_v_input2_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::p_v_output), aid_cast (buffer_test::p_v_output_parameter));
    
    // buffer_test::ap_v_input3_parameter = output_automaton->aid ();
    // buffer_test::ap_v_output_parameter = input_automaton->aid ();
    // rts::bind<buffer_test::ap_v_output_traits,
    // 	      buffer_test::ap_v_input3_traits> (output_automaton, &buffer_test::ap_v_output, buffer_test::ap_v_output_parameter,
    // 						input_automaton, &buffer_test::ap_v_input3, buffer_test::ap_v_input3_parameter);
    // checked_schedule (output_automaton, reinterpret_cast<const void*> (&buffer_test::ap_v_output), aid_cast (buffer_test::ap_v_output_parameter));
  }
}

extern "C" void
kmain (uint32_t multiboot_magic,
       multiboot_info_t* multiboot_info)  // Physical address.
{
  // Parse the multiboot data structure.
  multiboot_parser multiboot_parser (multiboot_magic, multiboot_info);

  // Initialize the system memory allocator so as to not stomp on the multiboot data structures.
  void* const heap_begin = align_up (std::max (static_cast<void*> (&data_end), reinterpret_cast<void*> (reinterpret_cast<size_t> (KERNEL_VIRTUAL_BASE) + multiboot_parser.end ())), PAGE_SIZE);
  const void* const heap_end = INITIAL_LOGICAL_LIMIT;
  system_alloc::initialize (heap_begin, heap_end);

  // Call the static constructors.
  // Static objects can use dynamic memory!!
  for (int** ptr = &ctors_begin; ptr != &ctors_end; ++ptr) {
    void (*ctor) () = reinterpret_cast<void (*) ()> (*ptr);
    ctor ();
  }

  // Print a welcome message.
  kout << "Lily" << endl;

  kout << "Multiboot data (physical) [" << hexformat (multiboot_parser.begin ()) << ", " << hexformat (multiboot_parser.end ()) << ")" << endl;
  kout << "Multiboot data  (logical) [" << hexformat (reinterpret_cast<physical_address_t> (KERNEL_VIRTUAL_BASE) + multiboot_parser.begin ()) << ", " << hexformat (reinterpret_cast<physical_address_t> (KERNEL_VIRTUAL_BASE) + multiboot_parser.end ()) << ")" << endl;

  kout << "Initial heap [" << hexformat (heap_begin) << ", " << hexformat (heap_end) << ")" << endl;

  // Set up segmentation for x86, or rather, ignore it.
  kout << "Installing GDT" << endl;
  gdt::install ();

  // Set up interrupt dispatching.
  kout << "Installing IDT" << endl;
  idt::install ();  
  kout << "Installing exception handler" << endl;
  exception_handler::install ();
  kout << "Installing irq handler" << endl;
  irq_handler::install ();
  kout << "Installing trap handler" << endl;
  trap_handler::install ();

  // Initialize the system that allocates frames (physical pages of memory).
  kout << "Memory map:" << endl;
  for (multiboot_parser::memory_map_iterator pos = multiboot_parser.memory_map_begin ();
       pos != multiboot_parser.memory_map_end ();
       ++pos) {
    kout << hexformat (static_cast<unsigned long> (pos->addr)) << "-" << hexformat (static_cast<unsigned long> (pos->addr + pos->len - 1));
    switch (pos->type) {
    case MULTIBOOT_MEMORY_AVAILABLE:
      kout << " AVAILABLE" << endl;
      {
	uint64_t begin = std::max (static_cast<multiboot_uint64_t> (USABLE_MEMORY_BEGIN), pos->addr);
	uint64_t end = std::min (static_cast<multiboot_uint64_t> (USABLE_MEMORY_END), pos->addr + pos->len);
	begin = align_down (begin, PAGE_SIZE);
	end = align_up (end, PAGE_SIZE);
	if (begin < end) {
	  frame_manager::add (pos->addr, pos->addr + pos->len);
	}
      }
      break;
    case MULTIBOOT_MEMORY_RESERVED:
      kout << " RESERVED" << endl;
      break;
    default:
      kout << " UNKNOWN" << endl;
      break;
    }
  }

  bool r;
  vm_area_interface* area;
  
  // Allocate the system automaton.
  automaton* sa = rts::create (descriptor_constants::RING0, vm::page_directory_frame ());
  
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
  system_automaton::system_automaton = sa;
  
  // We can't allocate any more memory until the page tables are corrected.
  vm::initialize (system_automaton::system_automaton);
  
  /* Add the actions. */
  system_automaton::system_automaton->add_action<system_automaton::init_traits> (&system_automaton::init);
  
  //create_action_test ();
  create_buffer_test ();
  
  // Start the scheduler.  Doesn't return.
  scheduler::finish (false, 0);

  kassert (0);
}
