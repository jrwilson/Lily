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
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "kout.hpp"
#include "automaton.hpp"
#include "scheduler.hpp"
#include "rts.hpp"

#include "action_test_automaton.hpp"
#include "buffer_test_automaton.hpp"

// Symbols to build memory maps.
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

struct allocator_tag { };
typedef list_alloc<allocator_tag> alloc_type;

template <class T>
typename list_alloc<T>::data list_alloc<T>::data_;

template <typename T>
struct allocator_type : public list_allocator<T, allocator_tag> { };

namespace system_automaton {

  // Heap location for system automata.
  // Starting at 4MB allow us to use the low page table of the kernel.
  static const logical_address_t SYSTEM_HEAP_BEGIN = FOUR_MEGABYTES;
  
  // Stack range for system automata.
  static const logical_address_t SYSTEM_STACK_BEGIN = KERNEL_VIRTUAL_BASE - PAGE_SIZE;
  static const logical_address_t SYSTEM_STACK_END = KERNEL_VIRTUAL_BASE;

  static automaton* system_automaton_ = 0;

  static void
  checked_schedule (automaton* a,
		    const void* aep,
		    aid_t p = 0)
  {
    automaton::const_action_iterator pos = a->action_find (aep);
    kassert (pos != a->action_end ());
    scheduler::schedule (caction (a, *pos, p));
  }

  void
  create_system_automaton ()
  {
    // Create the automaton.
    // Automaton can execute privileged instructions.  (Needed when system allocator calls invlpg when mapping).
    // Automaton can manipulate virtual memory.  (Could be done with buffers but this way is more direct.)
    // Automaton can access kernel data.  (That is where the code/data reside.)
    system_automaton_ = rts::create (true, vm::USER, vm::USER);

    // Create the memory map.
    vm_area_base* area;
    bool r;
    
    // Text.
    area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
					       reinterpret_cast<logical_address_t> (&text_end));
    r = system_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Read-only data.
    area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
						 reinterpret_cast<logical_address_t> (&rodata_end));
    r = system_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Data.
    area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
					       reinterpret_cast<logical_address_t> (&data_end));
    r = system_automaton_->insert_vm_area (area);
    kassert (r);

    // Heap.
    vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
    r = system_automaton_->insert_heap_area (heap_area);
    kassert (r);
    
    // Stack.
    vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
    r = system_automaton_->insert_stack_area (stack_area);
    kassert (r);
    
    // Add the actions.
    system_automaton_->add_action<system_automaton::first_traits> (&system_automaton::first);
    system_automaton_->add_action<system_automaton::init_traits> (&system_automaton::init);
    
    // Bootstrap.
    checked_schedule (system_automaton_, reinterpret_cast<const void*> (&system_automaton::first));
  }

  static void
  create_action_test_automaton ()
  {
    // Create automata to test actions.
    automaton* input_automaton;
    automaton* output_automaton;

    {    
      // Create the automaton.
      // Can't execute privileged instructions.
      // Can't manipulate virtual memory.
      // Can access kernel code/data.
      input_automaton = rts::create (false, vm::SUPERVISOR, vm::USER);
      
      // Create the memory map.
      vm_area_base* area;
      bool r;

      // Text.
      area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
						 reinterpret_cast<logical_address_t> (&text_end));
      r = input_automaton->insert_vm_area (area);
      kassert (r);
      
      // Read-only data.
      area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
						   reinterpret_cast<logical_address_t> (&rodata_end));
      r = input_automaton->insert_vm_area (area);
      kassert (r);
      
      // Data.
      area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
						 reinterpret_cast<logical_address_t> (&data_end));
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
      
      // Fourth, add the actions.
      input_automaton->add_action<action_test::np_nb_nc_input1_traits> (&action_test::np_nb_nc_input1);
      input_automaton->add_action<action_test::np_nb_nc_input2_traits> (&action_test::np_nb_nc_input2);
      input_automaton->add_action<action_test::np_nb_nc_input3_traits> (&action_test::np_nb_nc_input3);
      input_automaton->add_action<action_test::np_nb_c_input1_traits> (&action_test::np_nb_c_input1);
      input_automaton->add_action<action_test::np_nb_c_input2_traits> (&action_test::np_nb_c_input2);
      input_automaton->add_action<action_test::np_nb_c_input3_traits> (&action_test::np_nb_c_input3);
      input_automaton->add_action<action_test::np_b_nc_input1_traits> (&action_test::np_b_nc_input1);
      input_automaton->add_action<action_test::np_b_nc_input2_traits> (&action_test::np_b_nc_input2);
      input_automaton->add_action<action_test::np_b_nc_input3_traits> (&action_test::np_b_nc_input3);
      input_automaton->add_action<action_test::np_b_c_input1_traits> (&action_test::np_b_c_input1);
      input_automaton->add_action<action_test::np_b_c_input2_traits> (&action_test::np_b_c_input2);
      input_automaton->add_action<action_test::np_b_c_input3_traits> (&action_test::np_b_c_input3);
      input_automaton->add_action<action_test::p_nb_nc_input1_traits> (&action_test::p_nb_nc_input1);
      input_automaton->add_action<action_test::p_nb_nc_input2_traits> (&action_test::p_nb_nc_input2);
      input_automaton->add_action<action_test::p_nb_nc_input3_traits> (&action_test::p_nb_nc_input3);
      input_automaton->add_action<action_test::p_nb_c_input1_traits> (&action_test::p_nb_c_input1);
      input_automaton->add_action<action_test::p_nb_c_input2_traits> (&action_test::p_nb_c_input2);
      input_automaton->add_action<action_test::p_nb_c_input3_traits> (&action_test::p_nb_c_input3);
      input_automaton->add_action<action_test::p_b_nc_input1_traits> (&action_test::p_b_nc_input1);
      input_automaton->add_action<action_test::p_b_nc_input2_traits> (&action_test::p_b_nc_input2);
      input_automaton->add_action<action_test::p_b_nc_input3_traits> (&action_test::p_b_nc_input3);
      input_automaton->add_action<action_test::p_b_c_input1_traits> (&action_test::p_b_c_input1);
      input_automaton->add_action<action_test::p_b_c_input2_traits> (&action_test::p_b_c_input2);
      input_automaton->add_action<action_test::p_b_c_input3_traits> (&action_test::p_b_c_input3);
      input_automaton->add_action<action_test::ap_nb_nc_input1_traits> (&action_test::ap_nb_nc_input1);
      input_automaton->add_action<action_test::ap_nb_nc_input2_traits> (&action_test::ap_nb_nc_input2);
      input_automaton->add_action<action_test::ap_nb_nc_input3_traits> (&action_test::ap_nb_nc_input3);
      input_automaton->add_action<action_test::ap_nb_c_input1_traits> (&action_test::ap_nb_c_input1);
      input_automaton->add_action<action_test::ap_nb_c_input2_traits> (&action_test::ap_nb_c_input2);
      input_automaton->add_action<action_test::ap_nb_c_input3_traits> (&action_test::ap_nb_c_input3);
      input_automaton->add_action<action_test::ap_b_nc_input1_traits> (&action_test::ap_b_nc_input1);
      input_automaton->add_action<action_test::ap_b_nc_input2_traits> (&action_test::ap_b_nc_input2);
      input_automaton->add_action<action_test::ap_b_nc_input3_traits> (&action_test::ap_b_nc_input3);
      input_automaton->add_action<action_test::ap_b_c_input1_traits> (&action_test::ap_b_c_input1);
      input_automaton->add_action<action_test::ap_b_c_input2_traits> (&action_test::ap_b_c_input2);
      input_automaton->add_action<action_test::ap_b_c_input3_traits> (&action_test::ap_b_c_input3);
    }

    {
      // Create the automaton.
      // Can't execute privileged instructions.
      // Can't manipulate virtual memory.
      // Can access kernel code/data.
      output_automaton = rts::create (false, vm::SUPERVISOR, vm::USER);
      
      // Create the automaton's memory map.
      vm_area_base* area;
      bool r;

      // Text.
      area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
						 reinterpret_cast<logical_address_t> (&text_end));
      r = output_automaton->insert_vm_area (area);
      kassert (r);
      
      // Read-only data.
      area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
						   reinterpret_cast<logical_address_t> (&rodata_end));
      r = output_automaton->insert_vm_area (area);
      kassert (r);
      
      // Data.
      area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
						 reinterpret_cast<logical_address_t> (&data_end));
      r = output_automaton->insert_vm_area (area);
      kassert (r);
      
      // Heap.
      vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
      r = output_automaton->insert_heap_area (heap_area);
      kassert (r);
      
      // Stack.
      vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
      r = output_automaton->insert_stack_area (stack_area);
      kassert (r);
      
      // Add the actions.
      output_automaton->add_action<action_test::init_traits> (&action_test::init);
      output_automaton->add_action<action_test::np_nb_nc_output_traits> (&action_test::np_nb_nc_output);
      output_automaton->add_action<action_test::np_nb_c_output_traits> (&action_test::np_nb_c_output);
      output_automaton->add_action<action_test::np_b_nc_output_traits> (&action_test::np_b_nc_output);
      output_automaton->add_action<action_test::np_b_c_output_traits> (&action_test::np_b_c_output);
      output_automaton->add_action<action_test::p_nb_nc_output_traits> (&action_test::p_nb_nc_output);
      output_automaton->add_action<action_test::p_nb_c_output_traits> (&action_test::p_nb_c_output);
      output_automaton->add_action<action_test::p_b_nc_output_traits> (&action_test::p_b_nc_output);
      output_automaton->add_action<action_test::p_b_c_output_traits> (&action_test::p_b_c_output);
      output_automaton->add_action<action_test::ap_nb_nc_output_traits> (&action_test::ap_nb_nc_output);
      output_automaton->add_action<action_test::ap_nb_c_output_traits> (&action_test::ap_nb_c_output);
      output_automaton->add_action<action_test::ap_b_nc_output_traits> (&action_test::ap_b_nc_output);
      output_automaton->add_action<action_test::ap_b_c_output_traits> (&action_test::ap_b_c_output);
      
      output_automaton->add_action<action_test::np_internal_traits> (&action_test::np_internal);
      output_automaton->add_action<action_test::p_internal_traits> (&action_test::p_internal);

      // Bind.
      rts::bind<system_automaton::init_traits,
      	action_test::init_traits> (system_automaton_, &system_automaton::init, output_automaton,
      				   output_automaton, &action_test::init);
      checked_schedule (system_automaton_, reinterpret_cast<const void*> (&system_automaton::init), aid_cast (output_automaton));
      
      rts::bind<action_test::np_nb_nc_output_traits,
      	action_test::np_nb_nc_input1_traits> (output_automaton, &action_test::np_nb_nc_output,
      					      input_automaton, &action_test::np_nb_nc_input1);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_nb_nc_output));
      
      rts::bind<action_test::p_nb_nc_output_traits,
	action_test::np_nb_nc_input2_traits> (output_automaton, &action_test::p_nb_nc_output, action_test::p_nb_nc_output_parameter,
					      input_automaton, &action_test::np_nb_nc_input2);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_nb_nc_output), aid_cast (action_test::p_nb_nc_output_parameter));
      
      action_test::ap_nb_nc_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_nb_nc_output_traits,
	action_test::np_nb_nc_input3_traits> (output_automaton, &action_test::ap_nb_nc_output, action_test::ap_nb_nc_output_parameter,
					      input_automaton, &action_test::np_nb_nc_input3);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_nb_nc_output), aid_cast (action_test::ap_nb_nc_output_parameter));
      
      rts::bind<action_test::np_nb_c_output_traits,
	action_test::np_nb_c_input1_traits> (output_automaton, &action_test::np_nb_c_output,
					     input_automaton, &action_test::np_nb_c_input1);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_nb_c_output));
      
      rts::bind<action_test::p_nb_c_output_traits,
	action_test::np_nb_c_input2_traits> (output_automaton, &action_test::p_nb_c_output, action_test::p_nb_c_output_parameter,
					     input_automaton, &action_test::np_nb_c_input2);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_nb_c_output), aid_cast (action_test::p_nb_c_output_parameter));
      
      action_test::ap_nb_c_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_nb_c_output_traits,
	action_test::np_nb_c_input3_traits> (output_automaton, &action_test::ap_nb_c_output, action_test::ap_nb_c_output_parameter,
					     input_automaton, &action_test::np_nb_c_input3);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_nb_c_output), aid_cast (action_test::ap_nb_c_output_parameter));
      
      rts::bind<action_test::np_b_nc_output_traits,
	action_test::np_b_nc_input1_traits> (output_automaton, &action_test::np_b_nc_output,
					     input_automaton, &action_test::np_b_nc_input1);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_b_nc_output));
      
      rts::bind<action_test::p_b_nc_output_traits,
	action_test::np_b_nc_input2_traits> (output_automaton, &action_test::p_b_nc_output, action_test::p_b_nc_output_parameter,
					     input_automaton, &action_test::np_b_nc_input2);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_b_nc_output), aid_cast (action_test::p_b_nc_output_parameter));
      
      action_test::ap_b_nc_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_b_nc_output_traits,
	action_test::np_b_nc_input3_traits> (output_automaton, &action_test::ap_b_nc_output, action_test::ap_b_nc_output_parameter,
					     input_automaton, &action_test::np_b_nc_input3);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_b_nc_output), aid_cast (action_test::ap_b_nc_output_parameter));
      
      rts::bind<action_test::np_b_c_output_traits,
	action_test::np_b_c_input1_traits> (output_automaton, &action_test::np_b_c_output,
					    input_automaton, &action_test::np_b_c_input1);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_b_c_output));
      
      rts::bind<action_test::p_b_c_output_traits,
	action_test::np_b_c_input2_traits> (output_automaton, &action_test::p_b_c_output, action_test::p_b_c_output_parameter,
					    input_automaton, &action_test::np_b_c_input2);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_b_c_output), aid_cast (action_test::p_b_c_output_parameter));
      
      action_test::ap_b_c_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_b_c_output_traits,
	action_test::np_b_c_input3_traits> (output_automaton, &action_test::ap_b_c_output, action_test::ap_b_c_output_parameter,
					    input_automaton, &action_test::np_b_c_input3);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_b_c_output), aid_cast (action_test::ap_b_c_output_parameter));
      
      rts::bind<action_test::np_nb_nc_output_traits,
	action_test::p_nb_nc_input1_traits> (output_automaton, &action_test::np_nb_nc_output,
					     input_automaton, &action_test::p_nb_nc_input1, action_test::p_nb_nc_input1_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_nb_nc_output));
      
      rts::bind<action_test::p_nb_nc_output_traits,
	action_test::p_nb_nc_input2_traits> (output_automaton, &action_test::p_nb_nc_output, action_test::p_nb_nc_output_parameter,
					     input_automaton, &action_test::p_nb_nc_input2, action_test::p_nb_nc_input2_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_nb_nc_output), aid_cast (action_test::p_nb_nc_output_parameter));
      
      action_test::ap_nb_nc_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_nb_nc_output_traits,
	action_test::p_nb_nc_input3_traits> (output_automaton, &action_test::ap_nb_nc_output, action_test::ap_nb_nc_output_parameter,
					     input_automaton, &action_test::p_nb_nc_input3, action_test::p_nb_nc_input3_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_nb_nc_output), aid_cast (action_test::ap_nb_nc_output_parameter));
      
      rts::bind<action_test::np_nb_c_output_traits,
	action_test::p_nb_c_input1_traits> (output_automaton, &action_test::np_nb_c_output,
					    input_automaton, &action_test::p_nb_c_input1, action_test::p_nb_c_input1_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_nb_c_output));
      
      rts::bind<action_test::p_nb_c_output_traits,
	action_test::p_nb_c_input2_traits> (output_automaton, &action_test::p_nb_c_output, action_test::p_nb_c_output_parameter,
					    input_automaton, &action_test::p_nb_c_input2, action_test::p_nb_c_input2_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_nb_c_output), aid_cast (action_test::p_nb_c_output_parameter));
      
      action_test::ap_nb_c_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_nb_c_output_traits,
	action_test::p_nb_c_input3_traits> (output_automaton, &action_test::ap_nb_c_output, action_test::ap_nb_c_output_parameter,
					    input_automaton, &action_test::p_nb_c_input3, action_test::p_nb_c_input3_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_nb_c_output), aid_cast (action_test::ap_nb_c_output_parameter));
      
      rts::bind<action_test::np_b_nc_output_traits,
	action_test::p_b_nc_input1_traits> (output_automaton, &action_test::np_b_nc_output,
					    input_automaton, &action_test::p_b_nc_input1, action_test::p_b_nc_input1_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_b_nc_output));
      
      rts::bind<action_test::p_b_nc_output_traits,
	action_test::p_b_nc_input2_traits> (output_automaton, &action_test::p_b_nc_output, action_test::p_b_nc_output_parameter,
					    input_automaton, &action_test::p_b_nc_input2, action_test::p_b_nc_input2_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_b_nc_output), aid_cast (action_test::p_b_nc_output_parameter));
      
      action_test::ap_b_nc_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_b_nc_output_traits,
	action_test::p_b_nc_input3_traits> (output_automaton, &action_test::ap_b_nc_output, action_test::ap_b_nc_output_parameter,
					    input_automaton, &action_test::p_b_nc_input3, action_test::p_b_nc_input3_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_b_nc_output), aid_cast (action_test::ap_b_nc_output_parameter));
      
      rts::bind<action_test::np_b_c_output_traits,
	action_test::p_b_c_input1_traits> (output_automaton, &action_test::np_b_c_output,
					   input_automaton, &action_test::p_b_c_input1, action_test::p_b_c_input1_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_b_c_output));
      
      rts::bind<action_test::p_b_c_output_traits,
	action_test::p_b_c_input2_traits> (output_automaton, &action_test::p_b_c_output, action_test::p_b_c_output_parameter,
					   input_automaton, &action_test::p_b_c_input2, action_test::p_b_c_input2_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_b_c_output), aid_cast (action_test::p_b_c_output_parameter));
      
      action_test::ap_b_c_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_b_c_output_traits,
	action_test::p_b_c_input3_traits> (output_automaton, &action_test::ap_b_c_output, action_test::ap_b_c_output_parameter,
					   input_automaton, &action_test::p_b_c_input3, action_test::p_b_c_input3_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_b_c_output), aid_cast (action_test::ap_b_c_output_parameter));
      
      action_test::ap_nb_nc_input1_parameter = output_automaton->aid ();
      rts::bind<action_test::np_nb_nc_output_traits,
	action_test::ap_nb_nc_input1_traits> (output_automaton, &action_test::np_nb_nc_output,
					      input_automaton, &action_test::ap_nb_nc_input1, action_test::ap_nb_nc_input1_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_nb_nc_output));
      
      action_test::ap_nb_nc_input2_parameter = output_automaton->aid ();
      rts::bind<action_test::p_nb_nc_output_traits,
	action_test::ap_nb_nc_input2_traits> (output_automaton, &action_test::p_nb_nc_output, action_test::p_nb_nc_output_parameter,
					      input_automaton, &action_test::ap_nb_nc_input2, action_test::ap_nb_nc_input2_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_nb_nc_output), aid_cast (action_test::p_nb_nc_output_parameter));
      
      action_test::ap_nb_nc_input3_parameter = output_automaton->aid ();
      action_test::ap_nb_nc_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_nb_nc_output_traits,
	action_test::ap_nb_nc_input3_traits> (output_automaton, &action_test::ap_nb_nc_output, action_test::ap_nb_nc_output_parameter,
					      input_automaton, &action_test::ap_nb_nc_input3, action_test::ap_nb_nc_input3_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_nb_nc_output), aid_cast (action_test::ap_nb_nc_output_parameter));
      
      action_test::ap_nb_c_input1_parameter = output_automaton->aid ();
      rts::bind<action_test::np_nb_c_output_traits,
	action_test::ap_nb_c_input1_traits> (output_automaton, &action_test::np_nb_c_output,
					     input_automaton, &action_test::ap_nb_c_input1, action_test::ap_nb_c_input1_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_nb_c_output));
      
      action_test::ap_nb_c_input2_parameter = output_automaton->aid ();
      rts::bind<action_test::p_nb_c_output_traits,
	action_test::ap_nb_c_input2_traits> (output_automaton, &action_test::p_nb_c_output, action_test::p_nb_c_output_parameter,
					     input_automaton, &action_test::ap_nb_c_input2, action_test::ap_nb_c_input2_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_nb_c_output), aid_cast (action_test::p_nb_c_output_parameter));
      
      action_test::ap_nb_c_input3_parameter = output_automaton->aid ();
      action_test::ap_nb_c_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_nb_c_output_traits,
	action_test::ap_nb_c_input3_traits> (output_automaton, &action_test::ap_nb_c_output, action_test::ap_nb_c_output_parameter,
					     input_automaton, &action_test::ap_nb_c_input3, action_test::ap_nb_c_input3_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_nb_c_output), aid_cast (action_test::ap_nb_c_output_parameter));
      
      action_test::ap_b_nc_input1_parameter = output_automaton->aid ();
      rts::bind<action_test::np_b_nc_output_traits,
	action_test::ap_b_nc_input1_traits> (output_automaton, &action_test::np_b_nc_output,
					     input_automaton, &action_test::ap_b_nc_input1, action_test::ap_b_nc_input1_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_b_nc_output));
      
      action_test::ap_b_nc_input2_parameter = output_automaton->aid ();
      rts::bind<action_test::p_b_nc_output_traits,
	action_test::ap_b_nc_input2_traits> (output_automaton, &action_test::p_b_nc_output, action_test::p_b_nc_output_parameter,
					     input_automaton, &action_test::ap_b_nc_input2, action_test::ap_b_nc_input2_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_b_nc_output), aid_cast (action_test::p_b_nc_output_parameter));
      
      action_test::ap_b_nc_input3_parameter = output_automaton->aid ();
      action_test::ap_b_nc_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_b_nc_output_traits,
	action_test::ap_b_nc_input3_traits> (output_automaton, &action_test::ap_b_nc_output, action_test::ap_b_nc_output_parameter,
					     input_automaton, &action_test::ap_b_nc_input3, action_test::ap_b_nc_input3_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_b_nc_output), aid_cast (action_test::ap_b_nc_output_parameter));
    
      action_test::ap_b_c_input1_parameter = output_automaton->aid ();
      rts::bind<action_test::np_b_c_output_traits,
	action_test::ap_b_c_input1_traits> (output_automaton, &action_test::np_b_c_output,
					    input_automaton, &action_test::ap_b_c_input1, action_test::ap_b_c_input1_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::np_b_c_output));
    
      action_test::ap_b_c_input2_parameter = output_automaton->aid ();
      rts::bind<action_test::p_b_c_output_traits,
	action_test::ap_b_c_input2_traits> (output_automaton, &action_test::p_b_c_output, action_test::p_b_c_output_parameter,
					    input_automaton, &action_test::ap_b_c_input2, action_test::ap_b_c_input2_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::p_b_c_output), aid_cast (action_test::p_b_c_output_parameter));
    
      action_test::ap_b_c_input3_parameter = output_automaton->aid ();
      action_test::ap_b_c_output_parameter = input_automaton->aid ();
      rts::bind<action_test::ap_b_c_output_traits,
	action_test::ap_b_c_input3_traits> (output_automaton, &action_test::ap_b_c_output, action_test::ap_b_c_output_parameter,
					    input_automaton, &action_test::ap_b_c_input3, action_test::ap_b_c_input3_parameter);
      checked_schedule (output_automaton, reinterpret_cast<const void*> (&action_test::ap_b_c_output), aid_cast (action_test::ap_b_c_output_parameter));
    }
  }

  static void
  create_buffer_test_automaton ()
  {
    // Create the automaton.
    // Can't execute privileged instructions.
    // Can't manipulate virtual memory.
    // Can access kernel code/data.
    automaton* buffer_test_automaton = rts::create (false, vm::SUPERVISOR, vm::USER);
    
    // Create the memory map.
    vm_area_base* area;
    bool r;
    
    // Text.
    area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
					       reinterpret_cast<logical_address_t> (&text_end));
    r = buffer_test_automaton->insert_vm_area (area);
    kassert (r);
    
    // Read-only data.
    area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
						 reinterpret_cast<logical_address_t> (&rodata_end));
    r = buffer_test_automaton->insert_vm_area (area);
    kassert (r);
    
    // Data.
    area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
						 reinterpret_cast<logical_address_t> (&data_end));
    r = buffer_test_automaton->insert_vm_area (area);
    kassert (r);
    
    // Heap.
    vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
    r = buffer_test_automaton->insert_heap_area (heap_area);
    kassert (r);
    
    // Stack.
    vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
    r = buffer_test_automaton->insert_stack_area (stack_area);
    kassert (r);
      
    // Add the actions.
    buffer_test_automaton->add_action<buffer_test::init_traits> (&buffer_test::init);

    // Bind.
    rts::bind<system_automaton::init_traits,
      buffer_test::init_traits> (system_automaton_, &system_automaton::init, buffer_test_automaton,
				 buffer_test_automaton, &buffer_test::init);
    checked_schedule (system_automaton_, reinterpret_cast<const void*> (&system_automaton::init), aid_cast (buffer_test_automaton));
  }

  typedef fifo_scheduler<allocator_type> scheduler_type;
  static scheduler_type* scheduler_ = 0;

  static void
  schedule ()
  { }

  void
  first (void)
  {
    // Initialize the allocator.
    alloc_type::initialize ();
    // Allocate a scheduler.
    scheduler_ = new (alloc_type ()) scheduler_type ();

    create_action_test_automaton ();
    create_buffer_test_automaton ();

    schedule ();
    scheduler_->finish<first_traits> ();
  }

  void
  init (automaton* p)
  {
    scheduler_->remove<init_traits> (&init, p);
    schedule ();
    scheduler_->finish<init_traits> (true);
  }
  
}

// static void
// create_ramdisk (physical_address_t begin,
// 		physical_address_t end)
// {
//   // First, create a new page directory.
  
//   // Reserve some logical address space for the page directory.
//   vm::page_directory* pd = static_cast<vm::page_directory*> (vm::get_stub ());
//   // Allocate a frame.
//   frame_t frame = frame_manager::alloc ();
//   // Map the page directory.
//   vm::map (pd, frame, vm::USER, vm::WRITABLE);
//   // Initialize the page directory with a copy of the current page directory.
//   new (pd) vm::page_directory (true);
//   // Unmap.
//   vm::unmap (pd);
//   // Drop the reference count from allocation.
//   size_t count = frame_manager::decref (frame);
//   kassert (count == 1);
  
//   // Second, create the automaton.
//   automaton* ramdisk_automaton = rts::create (frame);
  
//   // Third, create the automaton's memory map.
//   {
//     vm_area_base* area;
//     bool r;
    
//     // Text.
//     area = new (system_alloc ()) vm_text_area (&text_begin, &text_end);
//     r = ramdisk_automaton->insert_vm_area (area);
//     kassert (r);
    
//     // Read-only data.
//     area = new (system_alloc ()) vm_rodata_area (&rodata_begin, &rodata_end);
//     r = ramdisk_automaton->insert_vm_area (area);
//     kassert (r);
    
//     // Data.
//     area = new (system_alloc ()) vm_data_area (&data_begin, &data_end);
//     r = ramdisk_automaton->insert_vm_area (area);
//     kassert (r);
    
//     // Heap.
//     vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
//     r = ramdisk_automaton->insert_heap_area (heap_area);
//     kassert (r);
    
//     // Stack.
//     vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
//     r = ramdisk_automaton->insert_stack_area (stack_area);
//     kassert (r);
//   }
  
//   // Fourth, add the actions.
//   ramdisk_automaton->add_action<ramdisk::init_traits> (&ramdisk::init);
//   ramdisk_automaton->add_action<ramdisk::info_request_traits> (&ramdisk::info_request);
//   ramdisk_automaton->add_action<ramdisk::info_response_traits> (&ramdisk::info_response);
//   ramdisk_automaton->add_action<ramdisk::read_request_traits> (&ramdisk::read_request);
//   ramdisk_automaton->add_action<ramdisk::read_response_traits> (&ramdisk::read_response);
//   ramdisk_automaton->add_action<ramdisk::write_request_traits> (&ramdisk::write_request);
//   ramdisk_automaton->add_action<ramdisk::write_response_traits> (&ramdisk::write_response);
  
//   // Bind.
//   rts::bind<system_automaton::init_traits,
// 	    ramdisk::init_traits> (system_automaton_, &system_automaton::init, ramdisk_automaton,
// 				   ramdisk_automaton, &ramdisk::init);
//   checked_schedule (system_automaton_, reinterpret_cast<const void*> (&system_automaton::init), aid_cast (ramdisk_automaton));

//   buffer* b = new (system_alloc ()) buffer (physical_address_to_frame (begin), physical_address_to_frame (align_up (end, PAGE_SIZE)));
//   ramdisk::bid = ramdisk_automaton->buffer_create (*b);
// }
