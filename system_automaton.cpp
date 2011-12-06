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

#include "scheduler.hpp"
#include "binding_manager.hpp"
#include "automaton.hpp"
#include "fifo_scheduler.hpp"
#include "ramdisk_automaton.hpp"
#include "action_macros.hpp"
#include "kassert.hpp"

// Symbols to build the kernel's memory map.
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

namespace system_automaton {

  enum status {
    NORMAL,
    BOOTING,
  };
  
  /* Size of the stack used for "system" automata.
     Must be large enough for functions and interrupts. */
  static const size_t SYSTEM_STACK_SIZE = 0x8000;
  
  static status status_;
  static const uint8_t* boot_begin_;
  static const uint8_t* boot_end_;
  static list_alloc alloc_ (false); // Delay initialization.
  static binding_manager* binding_manager_;
  static scheduler* system_scheduler_;
  static automaton* system_automaton_;

  static fifo_scheduler* scheduler_;
  typedef std::deque<automaton*, list_allocator<automaton*> > init_queue_type;
  static init_queue_type* init_queue_;
  
  static void
  schedule ();

  static void
  remove (size_t action_entry_point,
	     void* parameter)
  {
    scheduler_->remove (action_entry_point, parameter);
  }
  
  static void
  finish (void* buffer)
  {
    scheduler_->finish (buffer);
  }
  
  void
  null (void*);
  typedef internal_action_traits<void*, &null> null_traits;
  
  static bool
  null_precondition (void*)
  {
    return true;
  }
  
  static void
  null_effect (void*)
  {
    // Do nothing but activate the scheduler.
    kputs (__func__); kputs ("\n");
  }
  
  void
  null (void* p)
  {
    internal_action<null_traits> (p, remove, null_precondition, null_effect, schedule, finish);
  }
  
  void
  init (automaton*);
  typedef output_action_traits<automaton*, int, &init> init_traits;
  
  static bool
  init_precondition (automaton* a)
  {
    return !init_queue_->empty () && init_queue_->front () == a;
  }
  
  static void
  init_effect (automaton*,
		  int& message)
  {
    init_queue_->pop_front ();
    message = 314;
    kputs (__func__); kputs (" produced "); kputx32 (message); kputs ("\n");
  }
  
  void
  init (automaton* p)
  {
    output_action<init_traits> (p, remove, init_precondition, init_effect, schedule, finish);
  }
  
  void
  create_request (void*,
		     int)
  {
    kassert (0);
  }
  typedef input_action_traits<void*, int, &create_request> create_request_traits;
  
  void
  create_response (void*)
  {
    kassert (0);
  }
  typedef output_action_traits<void*, int, &create_response> create_response_traits;
  
  void
  bind_request (void*,
		   int)
  {
    kassert (0);
  }
  typedef input_action_traits<void*, int, &bind_request> bind_request_traits;
  
  void
  bind_response (void*)
  {
    kassert (0);
  }
  typedef output_action_traits<void*, int, &bind_response> bind_response_traits;
  
  void
  unbind_request (void*,
		     int)
  {
    kassert (0);
  }
  typedef input_action_traits<void*, int, &unbind_request> unbind_request_traits;
  
  void
  unbind_response (void*)
  {
    kassert (0);
  }
  typedef output_action_traits<void*, int, &unbind_response> unbind_response_traits;
  
  void
  destroy_request (void*,
		      int)
  {
    kassert (0);
  }
  typedef input_action_traits<void*, int, &destroy_request> destroy_request_traits;

  void
  destroy_response (void*)
  {
    kassert (0);
  }
  typedef output_action_traits<void*, int, &destroy_response> destroy_response_traits;

  // For testing.
  void
  read_request (void*);
  typedef output_action_traits<void*, ramdisk::block_num, &read_request> read_request_traits;

  static bool
  read_request_precondition (void*)
  {
    return true;
  }

  static void
  read_request_effect (void*,
		       ramdisk::block_num& block_num)
  {
    block_num = 0;
  }

  void
  read_request (void* p)
  {
    output_action<read_request_traits> (p, remove, read_request_precondition, read_request_effect, schedule, finish);
  }
  
  static void
  schedule ()
  {
    if (!init_queue_->empty ()) {
      scheduler_->add<init_traits> (init_queue_->front ());
    }
    if (read_request_precondition (0)) {
      scheduler_->add<read_request_traits> (0);
    }
  }
  
  void
  run (const void* placement_begin,
       const void* placement_end)
  {
    status_ = BOOTING;

    boot_begin_ = static_cast<uint8_t*> (const_cast<void*> (align_down (placement_begin, PAGE_SIZE)));
    boot_end_ = static_cast<uint8_t*> (const_cast<void*> (align_up (placement_end, PAGE_SIZE)));

    // We delayed initialization of the allocator because we don't know when interrupts are enabled.
    // However, they are enabled now so reinitialize using placement new.
    new (&alloc_) list_alloc ();

    // Allocate the binding manager.
    binding_manager_ = new (alloc_) binding_manager (alloc_);

    // Allocate the scheduler.
    system_scheduler_ = new (alloc_) scheduler (alloc_, *binding_manager_);

    // The system automaton's ceiling is the start of the paging data structures.
    // This is also used as the stack pointer.
    system_automaton_ = new (alloc_) automaton (alloc_, descriptor_constants::RING0, vm_manager::page_directory_physical_address (), vm_manager::page_directory_logical_address (), vm_manager::page_directory_logical_address (), paging_constants::SUPERVISOR);

    // Create a memory map for the system automatom.
    // Reserve low memory.
    kassert (system_automaton_->insert_vm_area (vm_reserved_area (0, static_cast<const uint8_t*> (KERNEL_VIRTUAL_BASE) + reinterpret_cast<size_t> (ONE_MEGABYTE))));
    // Program text.
    kassert (system_automaton_->insert_vm_area (vm_text_area (&text_begin, &text_end, paging_constants::SUPERVISOR)));
    // Program read-only data.
    kassert (system_automaton_->insert_vm_area (vm_rodata_area (&rodata_begin, &rodata_end, paging_constants::SUPERVISOR)));
    // Program data.
    kassert (system_automaton_->insert_vm_area (vm_data_area (&data_begin, &data_end, paging_constants::SUPERVISOR)));

    /* Add a stack. */
    vm_stack_area stack (reinterpret_cast<uint8_t*> (vm_manager::page_directory_logical_address ()) - SYSTEM_STACK_SIZE,
			 vm_manager::page_directory_logical_address (),
			 paging_constants::SUPERVISOR);
    kassert (system_automaton_->insert_vm_area (stack));
    /* When we call finish_action below, we will switch to the new stack.
       If we don't back the stack with physical pages, the first stack operation will triple fault.
       The scenario that we must avoid is:
       1.  Stack operation (recall that exceptions/interrupts use the stack).
       2.  No stack so page fault (exception).
       3.  Page fault will try to use the stack, same result.
       4.  Triple fault.  Game over.
       
       So, we need to back the stack with physical pages.
    */
    const void* ptr;
    for (ptr = stack.begin (); ptr < stack.end (); ptr = static_cast<const uint8_t*> (ptr) + PAGE_SIZE) {
      vm_manager::map (ptr, frame_manager::alloc (), paging_constants::SUPERVISOR, paging_constants::WRITABLE);
    }
    
    /* Add the actions. */
    system_automaton_->add_action<null_traits> ();
    system_automaton_->add_action<init_traits> ();
    system_automaton_->add_action<create_request_traits> ();
    system_automaton_->add_action<create_response_traits> ();
    system_automaton_->add_action<bind_request_traits> ();
    system_automaton_->add_action<bind_response_traits> ();
    system_automaton_->add_action<unbind_request_traits> ();
    system_automaton_->add_action<unbind_response_traits> ();
    system_automaton_->add_action<destroy_request_traits> ();
    system_automaton_->add_action<destroy_response_traits> ();
    system_automaton_->add_action<read_request_traits> ();
    
    // Add the system automaton to the system scheduler.
    system_scheduler_->add_automaton (system_automaton_);
    
    // Allocate the data structures for the system automaton.
    scheduler_ = new (alloc_) fifo_scheduler (alloc_);
    init_queue_ = new (alloc_) init_queue_type (alloc_);
    
    /* Create the ramdisk automaton. */
    
    /* First, create a new page directory. */
    
    /* Reserve some logical address space for the page directory. */
    page_directory* pd = static_cast<page_directory*> (system_automaton_->reserve (PAGE_SIZE));
    kassert (pd != 0);
    // Allocate a frame.
    frame_t frame = frame_manager::alloc ();
    /* Map the page directory. */
    vm_manager::map (pd, frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE);
    /* Initialize the page directory with a copy of the current page directory. */
    pd->initialize_with_current (frame_to_physical_address (frame));
    /* Unmap. */
    vm_manager::unmap (pd);
    /* Unreserve. */
    system_automaton_->unreserve (pd);
    
    /* Switch to the new page directory. */
    vm_manager::switch_to_directory (frame_to_physical_address (frame));
    
    /* Second, create the automaton. */
  automaton* ramdisk = new (alloc_) automaton (alloc_, descriptor_constants::RING0, frame_to_physical_address (frame), KERNEL_VIRTUAL_BASE, KERNEL_VIRTUAL_BASE, paging_constants::SUPERVISOR);

    /* Third, create the automaton's memory map. */
    {
      // Reserve low memory.
      kassert (ramdisk->insert_vm_area (vm_reserved_area (0, ONE_MEGABYTE)));
      
      /* Add a stack area. */
      vm_stack_area stack (static_cast<const uint8_t*> (ramdisk->stack_pointer ()) - SYSTEM_STACK_SIZE,
			   ramdisk->stack_pointer (),
			   paging_constants::SUPERVISOR);
      kassert (ramdisk->insert_vm_area (stack));
      
      /* Back with physical pages.  See previous comment about the stack. */
      const void* ptr;
      for (ptr = stack.begin (); ptr < stack.end (); ptr = static_cast<const uint8_t*> (ptr) + PAGE_SIZE) {
	vm_manager::map (ptr, frame_manager::alloc (), paging_constants::SUPERVISOR, paging_constants::WRITABLE);
      }
    }
    
    /* Fourth, add the actions. */
    ramdisk->add_action<ramdisk::init_traits> ();
    ramdisk->add_action<ramdisk::read_request_traits> ();

    /* Fifth, bind. */
    binding_manager_->bind<init_traits, ramdisk::init_traits> (system_automaton_, ramdisk,
							       ramdisk, 0);

    binding_manager_->bind<read_request_traits, ramdisk::read_request_traits> (system_automaton_, 0,
									       ramdisk, 0);
    
    /* Sixth, add the automaton to the system scheduler. */
    system_scheduler_->add_automaton (ramdisk);

    /* Seventh, initialize the new automaton. */
    init_queue_->push_back (ramdisk);
    
    /* Create the VFS automaton. */
    // TODO

    /* Create the init automaton. */
    // TODO

    // Finish the memory map for the system automaton with data that has been allocated.
    vm_data_area d (boot_begin_, static_cast<const uint8_t*> (boot_end_) + PAGE_SIZE, paging_constants::SUPERVISOR);
    kassert (system_automaton_->insert_vm_area (d));
    status_ = NORMAL;

    // Add the first action to the scheduler.
    system_scheduler_->schedule_action (system_automaton_, null_traits::action_entry_point, 0);
    
    // Start the scheduler.  Doesn't return.
    system_scheduler_->finish_action (0, 0, 0);
  }

  void
  page_fault (const void* address,
	      uint32_t error,
	      registers* regs)
  {
    switch (status_) {
    case NORMAL:
      if (address < KERNEL_VIRTUAL_BASE) {
	// Use the automaton's memory map.
	system_scheduler_->current_automaton ()->page_fault (address, error, regs);
      }
      else {
	// Use our memory map.
	system_automaton_->page_fault (address, error, regs);
      }
      break;
    case BOOTING:
      kassert (address >= boot_begin_);
      kassert (address < boot_end_);
      
      /* Fault should come from not being present. */
      kassert ((error & PAGE_PROTECTION_ERROR) == 0);
      /* Fault should come from data. */
      kassert ((error & PAGE_INSTRUCTION_ERROR) == 0);
      /* Back the request with a frame. */
      vm_manager::map (address, frame_manager::alloc (), paging_constants::SUPERVISOR, paging_constants::WRITABLE);
      /* Clear the frame. */
      /* TODO:  This is a long operation.  Move it out of the interrupt handler. */
      memset (const_cast<void*> (align_down (address, PAGE_SIZE)), 0x00, PAGE_SIZE);
      break;
    }
  }

  void
  finish_action (size_t action_entry_point,
		 void* parameter,
		 void* buffer)
  {
    system_scheduler_->finish_action (action_entry_point, parameter, buffer);
  }

  void*
  alloc (size_t size)
  {
    switch (status_) {
    case NORMAL:
      kputs ("NORMAL\n");
      return system_scheduler_->current_automaton ()->alloc (size);
      break;
    case BOOTING:
      {
	void* retval = const_cast<void*> (static_cast<const void*> (boot_end_));
	boot_end_ += size;
	boot_end_ = static_cast<uint8_t*> (const_cast<void*> (align_up (boot_end_, PAGE_SIZE)));
	return retval;
      }
      break;
    }
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
  bad_message (void)
  {
    // TODO
    kassert (0);
  }

}
