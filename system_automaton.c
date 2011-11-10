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

#include "system_automaton.h"
#include "kassert.h"
#include "descriptor.h"
#include "hash_map.h"
#include "malloc.h"
#include "scheduler.h"
#include "frame_manager.h"
#include "vm_manager.h"
#include "fifo_scheduler.h"
#include "binding_manager.h"

/* Size of the stack used for "system" automata.
   Must be large enough for functions and interrupts. */
#define SYSTEM_STACK_SIZE 0x1000

#define SWITCH_STACK_SIZE 256

/* Locations to use for data segments of system automata. */
#define SYSTEM_DATA_BEGIN 0X00100000
#define SYSTEM_DATA_END   0X00101000

/* Markers for the kernel. */
extern int text_begin;
extern int text_end;
extern int data_begin;
extern int data_end;

extern int initrd_init;

static automaton_t* system_automaton = 0;
static list_allocator_t* system_allocator = 0;
static fifo_scheduler_t* scheduler = 0;
static unsigned int counter = 0;

static void
schedule ();

UP_INTERNAL (system_automaton_first);
UV_P_OUTPUT (system_automaton_init, automaton_t*);
UP_INTERNAL (system_automaton_increment);

static int
system_automaton_first_precondition ()
{
  return scheduler == 0;
}

static void
system_automaton_first_effect ()
{
  /* Allocate a scheduler. */
  scheduler = fifo_scheduler_allocate (system_allocator);

  binding_manager_initialize (system_allocator);

  /* Create the initrd automaton. */

  /* First, create a new page directory. */

  /* Reserve some logical address space for the page directory. */
  page_directory_t* page_directory = automaton_reserve (system_automaton, PAGE_SIZE);
  kassert (page_directory != 0);
  /* Allocate a frame. */
  frame_t frame = frame_manager_alloc ();
  /* Map the page directory. */
  vm_manager_map (page_directory, frame, SUPERVISOR, WRITABLE);
  /* Initialize the page directory with a copy of the current page directory. */
  page_directory_initialize_with_current (page_directory, FRAME_TO_ADDRESS (frame));
  /* Unmap. */
  vm_manager_unmap (page_directory);
  /* Unreserve. */
  automaton_unreserve (system_automaton, page_directory);

  /* Switch to the new page directory. */
  vm_manager_switch_to_directory (FRAME_TO_ADDRESS (frame));
  
  /* Second, create the automaton. */
  automaton_t* initrd = automaton_allocate (system_allocator,
					    RING0,
					    FRAME_TO_ADDRESS (frame),
					    (void*)KERNEL_VIRTUAL_BASE,
					    (void*)KERNEL_VIRTUAL_BASE,
					    SUPERVISOR);

  /* Third, create the automaton's memory map. */
  {
    /* Add a data area. */
    vm_area_t data;
    vm_area_initialize (&data,
			VM_AREA_DATA,
			(void*)SYSTEM_DATA_BEGIN,
			(void*)SYSTEM_DATA_END,
			SUPERVISOR,
			WRITABLE);
    kassert (automaton_insert_vm_area (initrd, &data) == 0);

    /* Add a stack area. */
    vm_area_t stack;
    vm_area_initialize (&stack,
			VM_AREA_STACK,
			(void*)PAGE_ALIGN_DOWN ((size_t)initrd->stack_pointer - SYSTEM_STACK_SIZE),
			(void*)PAGE_ALIGN_DOWN ((size_t)initrd->stack_pointer),
			SUPERVISOR,
			WRITABLE);
    kassert (automaton_insert_vm_area (initrd, &stack) == 0);
    /* Back with physical pages.  See comment in system_automaton_initialize (). */
    void* ptr;
    for (ptr = stack.begin; ptr < stack.end; ptr += PAGE_SIZE) {
      vm_manager_map (ptr, frame_manager_alloc (), SUPERVISOR, WRITABLE);
    }
  }

  /* Fourth, add the actions. */
  automaton_set_action_type (initrd, &initrd_init, INPUT);

  /* Fifth, bind. */
  binding_manager_bind (system_automaton, &system_automaton_init, (parameter_t)initrd,
  			initrd, &initrd_init, 0);

  /* Sixth, initialize the new automaton. */
  SCHEDULER_ADD (scheduler, &system_automaton_init, (parameter_t)initrd);

  /* Create the VFS automaton. */

  /* Create the init automaton. */

  kputs (__func__); kputs ("\n");
}

static void
system_automaton_first_schedule () {
  schedule ();
}

static int
system_automaton_init_precondition (automaton_t* automaton)
{
  return 1;
}

static void
system_automaton_init_effect (automaton_t* automaton)
{
  kputs (__func__); kputs (" "); kputp (automaton); kputs ("\n");
}

static void
system_automaton_init_schedule (automaton_t* automaton) {
  schedule ();
}

static int
system_automaton_increment_precondition ()
{
  return counter < 10;
}

static void
system_automaton_increment_effect ()
{
  kputs (__func__); kputs (" counter = "); kputx32 (counter); kputs ("\n");
  ++counter;
}

static void
system_automaton_increment_schedule () {
  schedule ();
}

static void
schedule ()
{
  if (system_automaton_first_precondition ()) {
    SCHEDULER_ADD (scheduler, &system_automaton_first, 0);
  }
  if (system_automaton_increment_precondition ()) {
    SCHEDULER_ADD (scheduler, &system_automaton_increment, 0);
  }
}

void
system_automaton_initialize (void* placement_begin,
			     void* placement_end)
{
  kassert (system_automaton == 0);

  /* For bootstrapping, pretend that an automaton is running. */

  /* Create a simple memory map. */
  vm_area_t text;
  vm_area_initialize (&text,
		      VM_AREA_TEXT,
		      (void*)PAGE_ALIGN_DOWN ((size_t)&text_begin),
		      (void*)PAGE_ALIGN_UP ((size_t)&text_end),
		      SUPERVISOR,
		      NOT_WRITABLE);
  vm_area_t data;
  vm_area_initialize (&data,
		      VM_AREA_DATA,
		      (void*)PAGE_ALIGN_DOWN ((size_t)&data_begin),
		      (void*)PAGE_ALIGN_UP ((size_t)&data_end),
		      SUPERVISOR,
		      WRITABLE);
  vm_area_t placement;
  vm_area_initialize (&placement,
		      VM_AREA_DATA,
		      (void*)PAGE_ALIGN_DOWN ((size_t)placement_begin),
		      (void*)PAGE_ALIGN_UP ((size_t)placement_end),
		      SUPERVISOR,
		      WRITABLE);
  text.prev = 0;
  text.next = &data;
  data.prev = &text;
  data.next = &placement;
  placement.prev = &data;
  placement.next = 0;

  /* Initialize an automaton. */
  automaton_t fake_automaton;
  automaton_initialize (&fake_automaton, RING0, vm_manager_page_directory_physical_address (), 0, vm_manager_page_directory_logical_address (), SUPERVISOR);
  fake_automaton.memory_map_begin = &text;
  fake_automaton.memory_map_end = &placement;

  /* Use it as the system automaton. */
  system_automaton = &fake_automaton;

  /* Initialize the scheduler so it thinks the system automaton is executing. */
  scheduler_initialize (&fake_automaton);

  /* Now, we can start allocating. */

  /* Allocate the allocator. */
  system_allocator = list_allocator_allocate ();
  kassert (system_allocator != 0);

  /* Allocate and set the switch stack for the scheduler. */
  void* switch_stack = list_allocator_alloc (system_allocator, SWITCH_STACK_SIZE);
  kassert (switch_stack != 0);
  scheduler_set_switch_stack (switch_stack, SWITCH_STACK_SIZE);

  /* Allocate the real system automaton.
     The system automaton's ceiling is the start of the paging data structures.
     This is also used as the stack pointer. */
  automaton_t* sys_automaton = automaton_allocate (system_allocator, RING0, vm_manager_page_directory_physical_address (), vm_manager_page_directory_logical_address (), vm_manager_page_directory_logical_address (), SUPERVISOR);
  
  {
    /* Create a memory map for the system automatom. */
    vm_area_t* ptr;
    for (ptr = &text; ptr != 0; ptr = ptr->next) {
      kassert (automaton_insert_vm_area (sys_automaton, ptr) == 0);
    }
  }
  {
    /* Add a stack. */
    vm_area_t stack;
    vm_area_initialize (&stack,
			VM_AREA_STACK,
			(void*)PAGE_ALIGN_DOWN ((size_t)vm_manager_page_directory_logical_address () - SYSTEM_STACK_SIZE),
			(void*)PAGE_ALIGN_DOWN ((size_t)vm_manager_page_directory_logical_address ()),
			SUPERVISOR,
			WRITABLE);
    kassert (automaton_insert_vm_area (sys_automaton, &stack) == 0);
    /* When call finish_action below, we will switch to the new stack.
       If we don't back the stack with physical pages, the first stack operation will triple fault.
       The scenario that we must avoid is:
       1.  Stack operation (recall that exceptions/interrupts use the stack).
       2.  No stack so page fault (exception).
       3.  Page fault will try to use the stack, same result.
       4.  Triple fault.  Game over.

       So, we need to back the stack with physical pages.
    */
    void* ptr;
    for (ptr = stack.begin; ptr < stack.end; ptr += PAGE_SIZE) {
      vm_manager_map (ptr, frame_manager_alloc (), SUPERVISOR, WRITABLE);
    }
  }

  /* Add the actions. */
  automaton_set_action_type (sys_automaton, &system_automaton_first, INTERNAL);
  automaton_set_action_type (sys_automaton, &system_automaton_init, OUTPUT);
  automaton_set_action_type (sys_automaton, &system_automaton_increment, INTERNAL);

  /* Set the system automaton. */
  system_automaton = sys_automaton;

  /* Go! */
  schedule_action (system_automaton, &system_automaton_first, 0);

  /* Start the scheduler.  Doesn't return. */
  finish_action (0, 0);

  kassert (0);
}

automaton_t*
system_automaton_get_instance (void)
{
  kassert (system_automaton != 0);
  return system_automaton;
}
