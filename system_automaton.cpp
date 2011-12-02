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


#include "fifo_scheduler.hpp"
#include "action_macros.hpp"
#include "kassert.hpp"
#include "automaton.hpp"
#include "kernel.hpp"
#include "exception_handler.hpp"
#include "irq_handler.hpp"
#include "trap_handler.hpp"
#include "scheduler.hpp"
#include "binding_manager.hpp"

// Symbols to build the kernel's memory map.
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

// Symbols for the actions.
extern int sa_null;
extern int sa_init;
extern int sa_create_request;
extern int sa_create_response;
extern int sa_bind_request;
extern int sa_bind_response;
extern int sa_unbind_request;
extern int sa_unbind_response;
extern int sa_destroy_request;
extern int sa_destroy_response;

extern int initrd_init;

enum status {
  NORMAL,
  BOOTING,
};

/* Size of the stack used for "system" automata.
   Must be large enough for functions and interrupts. */
static const size_t SYSTEM_STACK_SIZE = 0x1000;

/* Locations to use for data segments of system automata. */
static const logical_address SYSTEM_DATA_BEGIN (reinterpret_cast<void*> (0X00100000));
static const logical_address SYSTEM_DATA_END (reinterpret_cast<void*> (0X00101000));

static kernel kernel_;
static status status_;
static logical_address boot_begin_;
static logical_address boot_end_;
static list_alloc alloc_ (false);
static scheduler* system_scheduler_;
static automaton* system_automaton_;
static fifo_scheduler* scheduler_;
static binding_manager* binding_manager_;

static void
schedule ();

namespace system_automaton {
  void
  run (void)
  {
    exception_handler::install (kernel_.interrupt_descriptor_table_);
    irq_handler::install (kernel_.interrupt_descriptor_table_);
    trap_handler::install (kernel_.interrupt_descriptor_table_);

    status_ = BOOTING;
    boot_begin_ = kernel_.placement_alloc_.begin () >> PAGE_SIZE;
    boot_end_ = kernel_.placement_alloc_.end () << PAGE_SIZE;

    // We delayed initialization of the allocator because we don't know when interrupts are enabled.
    // However, they are enabled now so reinitialize using placement new.
    new (&alloc_) list_alloc ();

    // Allocate the binding manager.
    binding_manager_ = new (alloc_) binding_manager (alloc_);

    // Allocate the scheduler.
    system_scheduler_ = new (alloc_) scheduler (alloc_, kernel_.vm_manager_, *binding_manager_);

    // The system automaton's ceiling is the start of the paging data structures.
    // This is also used as the stack pointer.
    system_automaton_ = new (alloc_) automaton (alloc_, descriptor_constants::RING0, kernel_.vm_manager_.page_directory_physical_address (), kernel_.vm_manager_.page_directory_logical_address (), kernel_.vm_manager_.page_directory_logical_address (), paging_constants::SUPERVISOR);

    // Create a memory map for the system automatom.
    // Reserve low memory.
    kassert (system_automaton_->insert_vm_area (vm_reserved_area (logical_address (0), logical_address (KERNEL_VIRTUAL_BASE) + 0x100000)));
    // Program text.
    kassert (system_automaton_->insert_vm_area (vm_text_area (logical_address (&text_begin), logical_address (&text_end), paging_constants::SUPERVISOR)));
    // Program read-only data.
    kassert (system_automaton_->insert_vm_area (vm_rodata_area (logical_address (&rodata_begin), logical_address (&rodata_end), paging_constants::SUPERVISOR)));
    // Program data.
    kassert (system_automaton_->insert_vm_area (vm_data_area (logical_address (&data_begin), logical_address (&data_end), paging_constants::SUPERVISOR)));

    /* Add a stack. */
    vm_stack_area stack (kernel_.vm_manager_.page_directory_logical_address () - SYSTEM_STACK_SIZE,
			 kernel_.vm_manager_.page_directory_logical_address (),
			 paging_constants::SUPERVISOR);
    kassert (system_automaton_->insert_vm_area (stack));
    /* When call finish_action below, we will switch to the new stack.
       If we don't back the stack with physical pages, the first stack operation will triple fault.
       The scenario that we must avoid is:
       1.  Stack operation (recall that exceptions/interrupts use the stack).
       2.  No stack so page fault (exception).
       3.  Page fault will try to use the stack, same result.
       4.  Triple fault.  Game over.
     
       So, we need to back the stack with physical pages.
    */
    logical_address ptr;
    for (ptr = stack.begin (); ptr < stack.end (); ptr += PAGE_SIZE) {
      kernel_.vm_manager_.map (ptr, kernel_.frame_manager_.alloc (), paging_constants::SUPERVISOR, paging_constants::WRITABLE);
    }
  
    /* Add the actions. */
    system_automaton_->add_action (&sa_null, INTERNAL);
    system_automaton_->add_action (&sa_init, OUTPUT);
    system_automaton_->add_action (&sa_create_request, INPUT);
    system_automaton_->add_action (&sa_create_response, OUTPUT);
    system_automaton_->add_action (&sa_bind_request, INPUT);
    system_automaton_->add_action (&sa_bind_response, OUTPUT);
    system_automaton_->add_action (&sa_unbind_request, INPUT);
    system_automaton_->add_action (&sa_unbind_response, OUTPUT);
    system_automaton_->add_action (&sa_destroy_request, INPUT);
    system_automaton_->add_action (&sa_destroy_response, OUTPUT);

    // Add the system automaton to the system scheduler.
    system_scheduler_->add_automaton (system_automaton_);

    // Allocate the local scheduler.
    scheduler_ = new (alloc_) fifo_scheduler (alloc_);

    /* Create the initrd automaton. */
    
    /* First, create a new page directory. */
    
    /* Reserve some logical address space for the page directory. */
    page_directory* pd = static_cast<page_directory*> (system_automaton_->reserve (PAGE_SIZE).value ());
    kassert (pd != 0);
    // Allocate a frame.
    frame frame = kernel_.frame_manager_.alloc ();
    /* Map the page directory. */
    kernel_.vm_manager_.map (logical_address (pd), frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE);
    /* Initialize the page directory with a copy of the current page directory. */
    pd->initialize_with_current (kernel_.frame_manager_, frame);
    /* Unmap. */
    kernel_.vm_manager_.unmap (logical_address (pd));
    /* Unreserve. */
    system_automaton_->unreserve (logical_address (pd));
    
    /* Switch to the new page directory. */
    kernel_.vm_manager_.switch_to_directory (physical_address (frame));
    
    /* Second, create the automaton. */
    automaton* initrd = new (alloc_) automaton (alloc_, descriptor_constants::RING0, physical_address (frame), logical_address (KERNEL_VIRTUAL_BASE), logical_address (KERNEL_VIRTUAL_BASE), paging_constants::SUPERVISOR);

    /* Third, create the automaton's memory map. */
    {
      /* Add a data area. */
      kassert (initrd->insert_vm_area (vm_data_area (SYSTEM_DATA_BEGIN, SYSTEM_DATA_END, paging_constants::SUPERVISOR)));
      
      /* Add a stack area. */
      vm_stack_area stack (initrd->get_stack_pointer () - SYSTEM_STACK_SIZE,
			   initrd->get_stack_pointer (),
			   paging_constants::SUPERVISOR);
      kassert (initrd->insert_vm_area (stack));
      
      /* Back with physical pages.  See comment in initialize (). */
      logical_address ptr;
      for (ptr = stack.begin (); ptr < stack.end (); ptr += PAGE_SIZE) {
	kernel_.vm_manager_.map (ptr, kernel_.frame_manager_.alloc (), paging_constants::SUPERVISOR, paging_constants::WRITABLE);
      }
    }
    
    /* Fourth, add the actions. */
    initrd->add_action (&initrd_init, INPUT);

    /* Fifth, bind. */
    binding_manager_->bind (system_automaton_, &sa_init, reinterpret_cast<parameter_t> (initrd),
			    initrd, &initrd_init, 0);

    /* Sixth, add the automaton to the system scheduler. */
    system_scheduler_->add_automaton (initrd);

    /* Seventh, initialize the new automaton. */
    scheduler_->add (&sa_init, (parameter_t)initrd);
    
    /* Create the VFS automaton. */
    // TODO

    /* Create the init automaton. */
    // TODO

    // Finish the memory map for the system automaton with data that has been allocated.
    vm_data_area d (boot_begin_, boot_end_ + PAGE_SIZE, paging_constants::SUPERVISOR);
    kassert (system_automaton_->insert_vm_area (d));
    status_ = NORMAL;

    // Add the first action to the scheduler.
    system_scheduler_->schedule_action (system_automaton_, &sa_null, 0);
    
    // Start the scheduler.  Doesn't return.
    system_scheduler_->finish_action (false, 0);
  }

  void
  page_fault (logical_address const& address,
	      uint32_t error)
  {
    switch (status_) {
    case NORMAL:
      if (address < logical_address (KERNEL_VIRTUAL_BASE)) {
	// Use the automaton's memory map.
	system_scheduler_->current_automaton ()->page_fault (kernel_.frame_manager_, kernel_.vm_manager_, address, error);
      }
      else {
	// Use our memory map.
	system_automaton_->page_fault (kernel_.frame_manager_, kernel_.vm_manager_, address, error);
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
      kernel_.vm_manager_.map (address, kernel_.frame_manager_.alloc (), paging_constants::SUPERVISOR, paging_constants::WRITABLE);
      /* Clear the frame. */
      /* TODO:  This is a long operation.  Move it out of the interrupt handler. */
      memset (address.value (), 0x00, PAGE_SIZE);
      break;
    }
  }

  void
  schedule_action (void* action_entry_point,
		   parameter_t parameter)
  {
    system_scheduler_->schedule_action (system_scheduler_->current_automaton (), action_entry_point, parameter);
  }

  void
  finish_action (bool output_status,
		 value_t output_value)
  {
    system_scheduler_->finish_action (output_status, output_value);
  }

  logical_address
  alloc (size_t size)
  {
    switch (status_) {
    case NORMAL:
      return system_scheduler_->current_automaton ()->alloc (size);
      break;
    case BOOTING:
      {
	logical_address retval = boot_end_;
	boot_end_ += size;
	boot_end_ <<= PAGE_SIZE;
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

}

static bool
sa_null_precondition ()
{
  return true;
}

static void
sa_null_effect ()
{
  // Do nothing but activate the scheduler.
  kputs (__func__); kputs ("\n");
}

static void
sa_null_schedule () {
  schedule ();
}

static bool
sa_init_precondition (automaton*)
{
  return true;
}

static void
sa_init_effect (automaton* automaton)
{
  kputs (__func__); kputs (" "); kputp (automaton); kputs ("\n");
}

static void
sa_init_schedule (automaton*) {
  schedule ();
}

static void
sa_create_request_effect () {
  kassert (0);
}

static void
sa_create_request_schedule () {
  kassert (0);
}

static bool
sa_create_response_precondition () {
  kassert (0);
  return false;
}

static void
sa_create_response_effect () {
  kassert (0);
}

static void
sa_create_response_schedule () {
  kassert (0);
}

static void
sa_bind_request_effect () {
  kassert (0);
}

static void
sa_bind_request_schedule () {
  kassert (0);
}

static bool
sa_bind_response_precondition () {
  kassert (0);
  return false;
}

static void
sa_bind_response_effect () {
  kassert (0);
}

static void
sa_bind_response_schedule () {
  kassert (0);
}

static void
sa_unbind_request_effect () {
  kassert (0);
}

static void
sa_unbind_request_schedule () {
  kassert (0);
}

static bool
sa_unbind_response_precondition () {
  kassert (0);
  return false;
}

static void
sa_unbind_response_effect () {
  kassert (0);
}

static void
sa_unbind_response_schedule () {
  kassert (0);
}

static void
sa_destroy_request_effect () {
  kassert (0);
}

static void
sa_destroy_request_schedule () {
  kassert (0);
}

static bool
sa_destroy_response_precondition () {
  kassert (0);
  return false;
}

static void
sa_destroy_response_effect () {
  kassert (0);
}

static void
sa_destroy_response_schedule () {
  kassert (0);
}

static void
schedule ()
{

}

static void
schedule_remove (void* action_entry_point,
		 parameter_t parameter)
{
  scheduler_->remove (action_entry_point, parameter);
}

static void
schedule_finish (bool output_status,
		 value_t output_value)
{
  scheduler_->finish (output_status, output_value);
}

UP_INTERNAL (sa_null);
UV_P_OUTPUT (sa_init, automaton*);
UV_UP_INPUT (sa_create_request);
UV_UP_OUTPUT (sa_create_response);
UV_UP_INPUT (sa_bind_request);
UV_UP_OUTPUT (sa_bind_response);
UV_UP_INPUT (sa_unbind_request);
UV_UP_OUTPUT (sa_unbind_response);
UV_UP_INPUT (sa_destroy_request);
UV_UP_OUTPUT (sa_destroy_response);
