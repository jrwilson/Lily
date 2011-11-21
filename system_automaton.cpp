/*
  File
  ----
  system_automaton.h
  
  Description
  -----------
  The system automaton.

  Authors:
  Justin R. Wilson
*/

#include "system_automaton.hpp"
#include "kassert.hpp"
#include "descriptor.hpp"
#include "scheduler.hpp"
#include "frame_manager.hpp"
#include "vm_manager.hpp"
#include "fifo_scheduler.hpp"
#include "binding_manager.hpp"
#include "boot_automaton.hpp"
#include <new>
#include "action_macros.hpp"

/* Size of the stack used for "system" automata.
   Must be large enough for functions and interrupts. */
#define SYSTEM_STACK_SIZE 0x1000

#define SWITCH_STACK_SIZE 256

/* Locations to use for data segments of system automata. */
const logical_address SYSTEM_DATA_BEGIN (reinterpret_cast<void*> (0X00100000));
const logical_address SYSTEM_DATA_END (reinterpret_cast<void*> (0X00101000));

/* Markers for the kernel. */
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

extern int initrd_init;

static automaton_interface* system_automaton = 0;
static list_allocator system_allocator;

static fifo_scheduler* scheduler = 0;
static bool flag = true;

static void
schedule ();

UP_INTERNAL (system_automaton_first, *scheduler);
UV_P_OUTPUT (system_automaton_init, automaton*, *scheduler);
UV_UP_INPUT (system_automaton_create_request, *scheduler);
UV_UP_OUTPUT (system_automaton_create_response, *scheduler);
UV_UP_INPUT (system_automaton_bind_request, *scheduler);
UV_UP_OUTPUT (system_automaton_bind_response, *scheduler);
UV_UP_INPUT (system_automaton_unbind_request, *scheduler);
UV_UP_OUTPUT (system_automaton_unbind_response, *scheduler);
UV_UP_INPUT (system_automaton_destroy_request, *scheduler);
UV_UP_OUTPUT (system_automaton_destroy_response, *scheduler);

static bool
system_automaton_first_precondition ()
{
  return flag;
}

static void
system_automaton_first_effect ()
{
  /* Create the initrd automaton. */

  /* First, create a new page directory. */

  /* Reserve some logical address space for the page directory. */
  page_directory_t* page_directory = static_cast<page_directory_t*> (system_automaton->reserve (PAGE_SIZE).value ());
  kassert (page_directory != 0);
  /* Allocate a frame. */
  frame frame = frame_manager::alloc ();
  /* Map the page directory. */
  vm_manager_map (logical_address (page_directory), frame, SUPERVISOR, WRITABLE);
  /* Initialize the page directory with a copy of the current page directory. */
  page_directory_initialize_with_current (page_directory, frame);
  /* Unmap. */
  vm_manager_unmap (logical_address (page_directory));
  /* Unreserve. */
  system_automaton->unreserve (logical_address (page_directory));

  /* Switch to the new page directory. */
  vm_manager_switch_to_directory (physical_address (frame));
  
  /* Second, create the automaton. */
  automaton* initrd = new (static_cast<automaton*> (system_allocator.alloc (sizeof (automaton)))) automaton (system_allocator, RING0, physical_address (frame), KERNEL_VIRTUAL_BASE, KERNEL_VIRTUAL_BASE, SUPERVISOR);

  /* Third, create the automaton's memory map. */
  {
    /* Add a data area. */
    vm_area data (VM_AREA_DATA,
		  SYSTEM_DATA_BEGIN,
		  SYSTEM_DATA_END,
		  SUPERVISOR);
    kassert (initrd->insert_vm_area (&data) == 0);

    /* Add a stack area. */
    vm_area stack (VM_AREA_STACK,
		   (initrd->get_stack_pointer () - SYSTEM_STACK_SIZE).align_down (PAGE_SIZE),
		   initrd->get_stack_pointer ().align_down (PAGE_SIZE),
		   SUPERVISOR);
    kassert (initrd->insert_vm_area (&stack) == 0);
    /* Back with physical pages.  See comment in system_automaton_initialize (). */
    logical_address ptr;
    for (ptr = stack.begin; ptr < stack.end; ptr += PAGE_SIZE) {
      vm_manager_map (ptr, frame_manager::alloc (), SUPERVISOR, WRITABLE);
    }
  }

  /* Fourth, add the actions. */
  initrd->add_action (&initrd_init, INPUT);

  /* Fifth, bind. */
  binding_manager::bind (system_automaton, &system_automaton_init, reinterpret_cast<parameter_t> (initrd),
			 initrd, &initrd_init, 0);
  
  /* Sixth, initialize the new automaton. */
  (*scheduler).add (&system_automaton_init, (parameter_t)initrd);

  /* Create the VFS automaton. */

  /* Create the init automaton. */

  kputs (__func__); kputs ("\n");

  flag = false;
}

static void
system_automaton_first_schedule () {
  schedule ();
}

static bool
system_automaton_init_precondition (automaton*)
{
  return true;
}

static void
system_automaton_init_effect (automaton* automaton)
{
  kputs (__func__); kputs (" "); kputp (automaton); kputs ("\n");
}

static void
system_automaton_init_schedule (automaton*) {
  schedule ();
}

static void
system_automaton_create_request_effect () {
  kassert (0);
}

static void
system_automaton_create_request_schedule () {
  kassert (0);
}

static bool
system_automaton_create_response_precondition () {
  kassert (0);
  return false;
}

static void
system_automaton_create_response_effect () {
  kassert (0);
}

static void
system_automaton_create_response_schedule () {
  kassert (0);
}

static void
system_automaton_bind_request_effect () {
  kassert (0);
}

static void
system_automaton_bind_request_schedule () {
  kassert (0);
}

static bool
system_automaton_bind_response_precondition () {
  kassert (0);
  return false;
}

static void
system_automaton_bind_response_effect () {
  kassert (0);
}

static void
system_automaton_bind_response_schedule () {
  kassert (0);
}

static void
system_automaton_unbind_request_effect () {
  kassert (0);
}

static void
system_automaton_unbind_request_schedule () {
  kassert (0);
}

static bool
system_automaton_unbind_response_precondition () {
  kassert (0);
  return false;
}

static void
system_automaton_unbind_response_effect () {
  kassert (0);
}

static void
system_automaton_unbind_response_schedule () {
  kassert (0);
}

static void
system_automaton_destroy_request_effect () {
  kassert (0);
}

static void
system_automaton_destroy_request_schedule () {
  kassert (0);
}

static bool
system_automaton_destroy_response_precondition () {
  kassert (0);
  return false;
}

static void
system_automaton_destroy_response_effect () {
  kassert (0);
}

static void
system_automaton_destroy_response_schedule () {
  kassert (0);
}


static void
schedule ()
{
  if (system_automaton_first_precondition ()) {
    (*scheduler).add (&system_automaton_first, 0);
  }
}

void
system_automaton_initialize (logical_address placement_begin,
			     logical_address placement_end)
{
  kassert (system_automaton == 0);

  /* To create the data structures for the system automaton and scheduler, we need dynamic memory allocation.
     All memory is allocated through a system call.
     System calls require that an automaton be executing.
     For an automaton to be executing, we need the data structures for the system automaton and scheduler.

     Uh oh.

     To break the recursion, we'll manually create an automaton that behaves correctly so that dynamic memory can be initialized.
  */

  boot_automaton boot_automaton (placement_begin, placement_end);

  /* Use it as the system automaton. */
  system_automaton = &boot_automaton;

  /* Initialize the scheduler so it thinks the system automaton is executing. */
  scheduler_initialize (&boot_automaton);

  /* Now, we can start allocating. */

  /* Allocate and set the switch stack for the scheduler. */
  void* switch_stack = system_allocator.alloc (SWITCH_STACK_SIZE);
  kassert (switch_stack != 0);
  scheduler_set_switch_stack (logical_address (switch_stack), SWITCH_STACK_SIZE);

  /* Allocate the real system automaton.
     The system automaton's ceiling is the start of the paging data structures.
     This is also used as the stack pointer. */
  automaton* sys_automaton = new (static_cast<automaton*> (system_allocator.alloc (sizeof (automaton)))) automaton (system_allocator, RING0, vm_manager_page_directory_physical_address (), vm_manager_page_directory_logical_address (), vm_manager_page_directory_logical_address (), SUPERVISOR);
  
  {
    /* Create a memory map for the system automatom. */
    vm_area text (VM_AREA_TEXT,
		  logical_address (&text_begin).align_down (PAGE_SIZE),
		  logical_address (&text_end).align_up (PAGE_SIZE),
		  SUPERVISOR);
    kassert (sys_automaton->insert_vm_area (&text) == 0);
    vm_area rodata (VM_AREA_DATA,
		    logical_address (&rodata_begin).align_down (PAGE_SIZE),
		    logical_address (&rodata_end).align_up (PAGE_SIZE),
		    SUPERVISOR);
    kassert (sys_automaton->insert_vm_area (&rodata) == 0);
    vm_area data (VM_AREA_DATA,
		  logical_address (&data_begin).align_down (PAGE_SIZE),
		  logical_address (&data_end).align_up (PAGE_SIZE),
		  SUPERVISOR);
    kassert (sys_automaton->insert_vm_area (&data) == 0);
    kassert (sys_automaton->insert_vm_area (boot_automaton.get_data_area ()) == 0);
  }
  {
    /* Add a stack. */
    vm_area stack (VM_AREA_STACK,
		   (vm_manager_page_directory_logical_address () - SYSTEM_STACK_SIZE).align_down (PAGE_SIZE),
		   (vm_manager_page_directory_logical_address ()).align_down (PAGE_SIZE),
		   SUPERVISOR);
    kassert (sys_automaton->insert_vm_area (&stack) == 0);
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
    for (ptr = stack.begin; ptr < stack.end; ptr += PAGE_SIZE) {
      vm_manager_map (ptr, frame_manager::alloc (), SUPERVISOR, WRITABLE);
    }
  }

  /* Add the actions. */
  sys_automaton->add_action (&system_automaton_first, INTERNAL);
  sys_automaton->add_action (&system_automaton_init, OUTPUT);
  sys_automaton->add_action (&system_automaton_create_request, INPUT);
  sys_automaton->add_action (&system_automaton_create_response, OUTPUT);
  sys_automaton->add_action (&system_automaton_bind_request, INPUT);
  sys_automaton->add_action (&system_automaton_bind_response, OUTPUT);
  sys_automaton->add_action (&system_automaton_unbind_request, INPUT);
  sys_automaton->add_action (&system_automaton_unbind_response, OUTPUT);
  sys_automaton->add_action (&system_automaton_destroy_request, INPUT);
  sys_automaton->add_action (&system_automaton_destroy_response, OUTPUT);

  /* Set the system automaton. */
  system_automaton = sys_automaton;

  /* Create a scheduler. */
  scheduler = new fifo_scheduler ();
  kassert (scheduler != 0);

  binding_manager::initialize ();

  /* Go! */
  schedule_action (system_automaton, &system_automaton_first, 0);

  /* Start the scheduler.  Doesn't return. */
  finish_action (0, 0);

  kassert (0);
}

automaton_interface*
system_automaton_get_instance (void)
{
  kassert (system_automaton != 0);
  return system_automaton;
}
