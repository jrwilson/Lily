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

#include "system_allocator.hpp"
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

  /* Size of the stack used for "system" automata.
     Must be large enough for functions and interrupts. */
  static const size_t SYSTEM_STACK_SIZE = 0x1000;
  
  static system_alloc alloc_; // Delay initialization.
  typedef binding_manager<system_alloc, system_allocator> binding_manager_type;
  static binding_manager_type* binding_manager_;
  typedef scheduler<system_alloc, system_allocator> system_scheduler_type;
  static system_scheduler_type* system_scheduler_;
  typedef automaton<system_alloc, system_allocator> automaton_type;
  static automaton_type* system_automaton_;

  typedef fifo_scheduler<system_alloc, system_allocator> scheduler_type;
  static scheduler_type* scheduler_;
  typedef std::deque<automaton_type*, system_allocator<automaton_type*> > init_queue_type;
  static init_queue_type* init_queue_;

  struct schedule {
    void
    operator () () const;
  };

  typedef up_internal_action_traits null_traits;
  
  struct null_precondition {
    inline bool
    operator() () const
    {
      return true;
    }
  };
  
  struct null_effect {
    void
    operator() () const
    {
      // Do nothing but activate the scheduler.
      kputs ("null_effect\n");
    }
  };
  
  static void
  null ()
  {
    internal_action<null_traits> (scheduler_->remove<null_traits> (&null), null_precondition (), null_effect (), schedule (), scheduler_->finish ());
  }
  
  typedef p_uv_output_action_traits<automaton_type*> init_traits;
  
  static bool
  init_precondition (automaton_type* a)
  {
    return !init_queue_->empty () && init_queue_->front () == a;
  }
  
  static void
  init_effect (automaton_type*)
  {
    init_queue_->pop_front ();
    kputs (__func__); kputs ("\n");
  }
  
  static void
  init (automaton_type*)
  {
    kassert (0);
    //output_action<init_traits> (p, remove, init_precondition, init_effect, schedule, finish);
  }

  void
  schedule::operator() () const
  {
    if (!init_queue_->empty ()) {
      scheduler_->add<init_traits> (&init, init_queue_->front ());
    }
  }

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
    
  void
  run (const void* placement_begin,
       const void* placement_end)
  {
    // Initialize the allocator.
    alloc_.boot (placement_begin, placement_end);

    // Allocate the binding manager.
    binding_manager_ = new (alloc_) binding_manager_type (alloc_);

    // Allocate the scheduler.
    system_scheduler_ = new (alloc_) system_scheduler_type (alloc_, *binding_manager_);

    // The system automaton's ceiling is the start of the paging data structures.
    // This is also used as the stack pointer.
    system_automaton_ = new (alloc_) automaton_type (alloc_, descriptor_constants::RING0, vm_manager::page_directory_physical_address (), vm_manager::page_directory_logical_address (), vm_manager::page_directory_logical_address (), paging_constants::SUPERVISOR);

    // Create a memory map for the system automatom.
    // Reserve low memory.
    kassert (system_automaton_->insert_vm_area (new (alloc_) vm_reserved_area (0, static_cast<const uint8_t*> (KERNEL_VIRTUAL_BASE) + reinterpret_cast<size_t> (ONE_MEGABYTE))));
    // Program text.
    kassert (system_automaton_->insert_vm_area (new (alloc_) vm_text_area (&text_begin, &text_end, paging_constants::SUPERVISOR)));
    // Program read-only data.
    kassert (system_automaton_->insert_vm_area (new (alloc_) vm_rodata_area (&rodata_begin, &rodata_end, paging_constants::SUPERVISOR)));
    // Program data.
    kassert (system_automaton_->insert_vm_area (new (alloc_) vm_data_area (&data_begin, &data_end, paging_constants::SUPERVISOR)));

    /* Add a stack. */
    vm_stack_area* stack = new (alloc_) vm_stack_area (reinterpret_cast<uint8_t*> (vm_manager::page_directory_logical_address ()) - SYSTEM_STACK_SIZE,
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
    stack->back_with_frames ();

    // Switch to normal since the memory map is built.
    alloc_.normal (system_automaton_);
    
    /* Add the actions. */
    system_automaton_->add_action<null_traits> (&null);
    system_automaton_->add_action<init_traits> (&init);
    
    // Add the system automaton to the system scheduler.
    system_scheduler_->add_automaton (system_automaton_);
    
    // Allocate the data structures for the system automaton.
    scheduler_ = new (alloc_) scheduler_type (alloc_);
    init_queue_ = new (alloc_) init_queue_type (alloc_);

    // Add the first action to the scheduler.
    system_scheduler_->schedule_action (system_automaton_, reinterpret_cast<void*> (&null), 0);

    //create_ramdisk ();
    
    // Start the scheduler.  Doesn't return.
    system_scheduler_->finish_action (0, 0, 0);
  }

  void
  page_fault (const void* address,
	      uint32_t error,
	      registers* regs)
  {
    automaton_type* current_automaton = system_scheduler_->current_automaton ();
    kassert (current_automaton != system_automaton_);

    if (address < KERNEL_VIRTUAL_BASE) {
      // Use the automaton's memory map.
      current_automaton->page_fault (address, error, regs);
    }
    else {
      // Use our memory map.
      system_automaton_->page_fault (address, error, regs);
    }
  }

  void
  finish_action (const void* action_entry_point,
		 aid_t parameter,
		 const void* buffer)
  {
    system_scheduler_->finish_action (action_entry_point, parameter, buffer);
  }

  void*
  alloc (size_t size)
  {
    automaton_type* current_automaton = system_scheduler_->current_automaton ();
    kassert (current_automaton != system_automaton_);

    return current_automaton->alloc (size);
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
