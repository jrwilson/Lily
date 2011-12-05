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
#include "initrd_automaton.hpp"
#include "action_macros.hpp"
#include "kassert.hpp"

// Symbols to build the kernel's memory map.
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

void
schedule ();

enum status {
  NORMAL,
  BOOTING,
};

/* Size of the stack used for "system" automata.
   Must be large enough for functions and interrupts. */
static const size_t SYSTEM_STACK_SIZE = 0x1000;

static const logical_address ONE_MEGABYTE (reinterpret_cast<const void*> (0x00100000));

static status status_;
static logical_address boot_begin_;
static logical_address boot_end_;
static list_alloc alloc_ (false); // Delay initialization.
static binding_manager* binding_manager_;
static scheduler* system_scheduler_;
static automaton* system_automaton_;

static fifo_scheduler* scheduler_;
typedef std::deque<automaton*, list_allocator<automaton*> > init_queue_type;
static init_queue_type* init_queue_;

void
sa_remove (local_func action_entry_point,
	   void* parameter)
{
  scheduler_->remove (action_entry_point, parameter);
}

void
sa_finish (bool status,
	   void* buffer)
{
  scheduler_->finish (status, buffer);
}

bool
sa_null_precondition (void*)
{
  return true;
}

void
sa_null_effect (void*)
{
  // Do nothing but activate the scheduler.
  kputs (__func__); kputs ("\n");
}

struct sa_null_tag { };
typedef internal_action<sa_null_tag, void*, sa_remove, sa_null_precondition, sa_null_effect, schedule, sa_finish> sa_null_type;
static sa_null_type sa_null;

bool
sa_init_precondition (automaton* a)
{
  return !init_queue_->empty () && init_queue_->front () == a;
}

void
sa_init_effect (automaton*,
		int& value)
{
  init_queue_->pop_front ();
  value = 314;
  kputs (__func__); kputs (" produced "); kputx32 (value); kputs ("\n");
}

struct sa_init_tag { };
typedef output_action<sa_init_tag, automaton*, int, sa_remove, sa_init_precondition, sa_init_effect, schedule, sa_finish> sa_init_type;
static sa_init_type sa_init;

void
sa_create_request_effect (void*,
			  int&)
{
  kassert (0);
}

struct sa_create_request_tag { };
typedef input_action<sa_create_request_tag, void*, int, sa_create_request_effect, schedule, sa_finish> sa_create_request_type;
static sa_create_request_type sa_create_request;

bool
sa_create_response_precondition (void*)
{
  kassert (0);
  return false;
}

void
sa_create_response_effect (void*,
			   int&)
{
  kassert (0);
}

struct sa_create_response_tag { };
typedef output_action<sa_create_response_tag, void*, int, sa_remove, sa_create_response_precondition, sa_create_response_effect, schedule, sa_finish> sa_create_response_type;
static sa_create_response_type sa_create_response;

void
sa_bind_request_effect (void*,
			  int&)
{
  kassert (0);
}

struct sa_bind_request_tag { };
typedef input_action<sa_bind_request_tag, void*, int, sa_bind_request_effect, schedule, sa_finish> sa_bind_request_type;
static sa_bind_request_type sa_bind_request;

bool
sa_bind_response_precondition (void*)
{
  kassert (0);
  return false;
}

void
sa_bind_response_effect (void*,
			   int&)
{
  kassert (0);
}

struct sa_bind_response_tag { };
typedef output_action<sa_bind_response_tag, void*, int, sa_remove, sa_bind_response_precondition, sa_bind_response_effect, schedule, sa_finish> sa_bind_response_type;
static sa_bind_response_type sa_bind_response;

void
sa_unbind_request_effect (void*,
			  int&)
{
  kassert (0);
}

struct sa_unbind_request_tag { };
typedef input_action<sa_unbind_request_tag, void*, int, sa_unbind_request_effect, schedule, sa_finish> sa_unbind_request_type;
static sa_unbind_request_type sa_unbind_request;

bool
sa_unbind_response_precondition (void*)
{
  kassert (0);
  return false;
}

void
sa_unbind_response_effect (void*,
			   int&)
{
  kassert (0);
}

struct sa_unbind_response_tag { };
typedef output_action<sa_unbind_response_tag, void*, int, sa_remove, sa_unbind_response_precondition, sa_unbind_response_effect, schedule, sa_finish> sa_unbind_response_type;
static sa_unbind_response_type sa_unbind_response;

void
sa_destroy_request_effect (void*,
			  int&)
{
  kassert (0);
}

struct sa_destroy_request_tag { };
typedef input_action<sa_destroy_request_tag, void*, int, sa_destroy_request_effect, schedule, sa_finish> sa_destroy_request_type;
static sa_destroy_request_type sa_destroy_request;

bool
sa_destroy_response_precondition (void*)
{
  kassert (0);
  return false;
}

void
sa_destroy_response_effect (void*,
			   int&)
{
  kassert (0);
}

struct sa_destroy_response_tag { };
typedef output_action<sa_destroy_response_tag, void*, int, sa_remove, sa_destroy_response_precondition, sa_destroy_response_effect, schedule, sa_finish> sa_destroy_response_type;
static sa_destroy_response_type sa_destroy_response;

void
schedule ()
{
  if (!init_queue_->empty ()) {
    scheduler_->add (&sa_init_type::action, init_queue_->front ());
  }
}

namespace system_automaton {
  void
  run (logical_address placement_begin,
       logical_address placement_end)
  {
    status_ = BOOTING;

    boot_begin_ = placement_begin >> PAGE_SIZE;
    boot_end_ = placement_end << PAGE_SIZE;

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
    kassert (system_automaton_->insert_vm_area (vm_reserved_area (logical_address (0), KERNEL_VIRTUAL_BASE + 0x100000)));
    // Program text.
    kassert (system_automaton_->insert_vm_area (vm_text_area (logical_address (&text_begin), logical_address (&text_end), paging_constants::SUPERVISOR)));
    // Program read-only data.
    kassert (system_automaton_->insert_vm_area (vm_rodata_area (logical_address (&rodata_begin), logical_address (&rodata_end), paging_constants::SUPERVISOR)));
    // Program data.
    kassert (system_automaton_->insert_vm_area (vm_data_area (logical_address (&data_begin), logical_address (&data_end), paging_constants::SUPERVISOR)));

    /* Add a stack. */
    vm_stack_area stack (vm_manager::page_directory_logical_address () - SYSTEM_STACK_SIZE,
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
    logical_address ptr;
    for (ptr = stack.begin (); ptr < stack.end (); ptr += PAGE_SIZE) {
      vm_manager::map (ptr, frame_manager::alloc (), paging_constants::SUPERVISOR, paging_constants::WRITABLE);
    }
    
    /* Add the actions. */
    system_automaton_->add_action (sa_null);
    system_automaton_->add_action (sa_init);
    system_automaton_->add_action (sa_create_request);
    system_automaton_->add_action (sa_create_response);
    system_automaton_->add_action (sa_bind_request);
    system_automaton_->add_action (sa_bind_response);
    system_automaton_->add_action (sa_unbind_request);
    system_automaton_->add_action (sa_unbind_response);
    system_automaton_->add_action (sa_destroy_request);
    system_automaton_->add_action (sa_destroy_response);
    
    // Add the system automaton to the system scheduler.
    system_scheduler_->add_automaton (system_automaton_);
    
    // Allocate the data structures for the system automaton.
    scheduler_ = new (alloc_) fifo_scheduler (alloc_);
    init_queue_ = new (alloc_) init_queue_type (alloc_);
    
    /* Create the initrd automaton. */
    
    /* First, create a new page directory. */
    
    /* Reserve some logical address space for the page directory. */
    page_directory* pd = static_cast<page_directory*> (system_automaton_->reserve (PAGE_SIZE).value ());
    kassert (pd != 0);
    // Allocate a frame.
    frame frame = frame_manager::alloc ();
    /* Map the page directory. */
    vm_manager::map (logical_address (pd), frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE);
    /* Initialize the page directory with a copy of the current page directory. */
    pd->initialize_with_current (frame);
    /* Unmap. */
    vm_manager::unmap (logical_address (pd));
    /* Unreserve. */
    system_automaton_->unreserve (logical_address (pd));
    
    /* Switch to the new page directory. */
    vm_manager::switch_to_directory (physical_address (frame));
    
    /* Second, create the automaton. */
  automaton* initrd = new (alloc_) automaton (alloc_, descriptor_constants::RING0, physical_address (frame), KERNEL_VIRTUAL_BASE, KERNEL_VIRTUAL_BASE, paging_constants::SUPERVISOR);

    /* Third, create the automaton's memory map. */
    {
      // Reserve low memory.
      kassert (initrd->insert_vm_area (vm_reserved_area (logical_address (0), ONE_MEGABYTE)));
      
      /* Add a stack area. */
      vm_stack_area stack (initrd->stack_pointer () - SYSTEM_STACK_SIZE,
			   initrd->stack_pointer (),
			   paging_constants::SUPERVISOR);
      kassert (initrd->insert_vm_area (stack));
      
      /* Back with physical pages.  See previous comment about the stack. */
      logical_address ptr;
      for (ptr = stack.begin (); ptr < stack.end (); ptr += PAGE_SIZE) {
	vm_manager::map (ptr, frame_manager::alloc (), paging_constants::SUPERVISOR, paging_constants::WRITABLE);
      }
    }
    
    /* Fourth, add the actions. */
    initrd->add_action (initrd_init);

    /* Fifth, bind. */
    binding_manager_->bind (system_automaton_, sa_init, initrd,
			    initrd, initrd_init, 0);

    /* Sixth, add the automaton to the system scheduler. */
    system_scheduler_->add_automaton (initrd);

    /* Seventh, initialize the new automaton. */
    init_queue_->push_back (initrd);
    
    /* Create the VFS automaton. */
    // TODO

    /* Create the init automaton. */
    // TODO

    // Finish the memory map for the system automaton with data that has been allocated.
    vm_data_area d (boot_begin_, boot_end_ + PAGE_SIZE, paging_constants::SUPERVISOR);
    kassert (system_automaton_->insert_vm_area (d));
    status_ = NORMAL;

    // Add the first action to the scheduler.
    system_scheduler_->schedule_action (system_automaton_, &sa_null_type::action, 0);
    
    // Start the scheduler.  Doesn't return.
    system_scheduler_->finish_action (false, 0, 0, false, 0);
  }

  void
  page_fault (logical_address const& address,
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
      memset ((address >> PAGE_SIZE).value (), 0x00, PAGE_SIZE);
      break;
    }
  }

  void
  finish_action (bool schedule_status,
		 local_func action_entry_point,
		 void* parameter,
		 bool output_status,
		 void* buffer)
  {
    system_scheduler_->finish_action (schedule_status, action_entry_point, parameter, output_status, buffer);
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
