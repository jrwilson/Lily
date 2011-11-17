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
#include "hash_map.hpp"
#include "scheduler.hpp"
#include "frame_manager.hpp"
#include "vm_manager.hpp"
#include "fifo_scheduler.hpp"
#include "binding_manager.hpp"
#include "boot_automaton.hpp"
#include "new.hpp"
#include "action_macros.hpp"
#include "vector.hpp"

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

static void
schedule ();

UP_INTERNAL (system_automaton_first, scheduler);
UV_P_OUTPUT (system_automaton_init, automaton*, scheduler);
UV_UP_INPUT (system_automaton_create_request, scheduler);
UV_UP_OUTPUT (system_automaton_create_response, scheduler);
UV_UP_INPUT (system_automaton_bind_request, scheduler);
UV_UP_OUTPUT (system_automaton_bind_response, scheduler);
UV_UP_INPUT (system_automaton_unbind_request, scheduler);
UV_UP_OUTPUT (system_automaton_unbind_response, scheduler);
UV_UP_INPUT (system_automaton_destroy_request, scheduler);
UV_UP_OUTPUT (system_automaton_destroy_response, scheduler);

static bool
system_automaton_first_precondition ()
{
  return scheduler == 0;
}

static void
system_automaton_first_effect ()
{
  /* Allocate a scheduler. */
  scheduler = new (system_allocator.alloc (sizeof (fifo_scheduler))) fifo_scheduler (system_allocator);

  binding_manager_initialize (&system_allocator);

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
    vm_area_t data;
    vm_area_initialize (&data,
			VM_AREA_DATA,
			SYSTEM_DATA_BEGIN,
			SYSTEM_DATA_END,
			SUPERVISOR);
    kassert (initrd->insert_vm_area (&data) == 0);

    /* Add a stack area. */
    vm_area_t stack;
    vm_area_initialize (&stack,
			VM_AREA_STACK,
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
  binding_manager_bind (system_automaton, &system_automaton_init, reinterpret_cast<parameter_t> (initrd),
  			initrd, &initrd_init, 0);

  /* Sixth, initialize the new automaton. */
  scheduler->add (&system_automaton_init, (parameter_t)initrd);

  /* Create the VFS automaton. */

  /* Create the init automaton. */

  kputs (__func__); kputs ("\n");
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
    scheduler->add (&system_automaton_first, 0);
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

  {
    vector<int> vec;
    kassert (vec.size () == 0);

    for (int i = 0; i < 10; ++i) {
      vec.push_back (i);
    }
    kassert (vec.size () == 10);
    
    vec.insert (vec.end (), 10, 100);
    kassert (vec.size () == 20);

    vec.pop_back ();
    kassert (vec.size () == 19);
  }

  {
    vector<int> vec;
    kassert (vec.max_size () != 0);
  }

  {
    vector<int> vec;

    for (int i = 0; i < 10; ++i) {
      vec.push_back (i);
    }

    vec.resize (5);
    vec.resize (8, 100);
    vec.resize (12);

    kassert (vec[0] == 0);
    kassert (vec[1] == 1);
    kassert (vec[2] == 2);
    kassert (vec[3] == 3);
    kassert (vec[4] == 4);
    kassert (vec[5] == 100);
    kassert (vec[6] == 100);
    kassert (vec[7] == 100);
    kassert (vec[8] == 0);
    kassert (vec[9] == 0);
    kassert (vec[10] == 0);
    kassert (vec[11] == 0);
  }

  {
    vector<int> vec;
    vec.reserve (100);
    kassert (vec.capacity () >= 100);
  }

  {
    vector<int> vec;
    int sum (0);

    for (int i = 1; i <= 10; ++i) {
      vec.push_back (i);
    }

    while (!vec.empty ()) {
      sum += vec.back ();
      vec.pop_back ();
    }

    kassert (sum == 55);
  }

  {
    int sz = 10;
    vector<int> vec (sz);
    
    for (int i = 0; i < sz; ++i) {
      vec[i] = i;
    }

    for (int i = 0; i < sz / 2; ++i) {
      int temp = vec[sz - 1 - i];
      vec[sz - 1 - i] = vec[i];
      vec[i] = temp;
    }

    kassert (vec[0] == 9);
    kassert (vec[1] == 8);
    kassert (vec[2] == 7);
    kassert (vec[3] == 6);
    kassert (vec[4] == 5);
    kassert (vec[5] == 4);
    kassert (vec[6] == 3);
    kassert (vec[7] == 2);
    kassert (vec[8] == 1);
    kassert (vec[9] == 0);
  }

  {
    int sz = 10;
    vector<int> vec (sz);
    
    for (int i = 0; i < sz; ++i) {
      vec.at (i) = i;
    }

    for (int i = 0; i < sz / 2; ++i) {
      int temp = vec.at (sz - 1 - i);
      vec.at (sz - 1 - i) = vec.at (i);
      vec.at (i) = temp;
    }

    kassert (vec.at (0) == 9);
    kassert (vec.at (1) == 8);
    kassert (vec.at (2) == 7);
    kassert (vec.at (3) == 6);
    kassert (vec.at (4) == 5);
    kassert (vec.at (5) == 4);
    kassert (vec.at (6) == 3);
    kassert (vec.at (7) == 2);
    kassert (vec.at (8) == 1);
    kassert (vec.at (9) == 0);
  }

  {
    vector<int> vec;
    
    vec.push_back (78);
    vec.push_back (16);

    vec.front () -= vec.back ();

    kassert (vec.front () == 62);
  }

  {
    vector<int> vec;

    vec.push_back (10);

    while (vec.back () != 0) {
      vec.push_back (vec.back () - 1);
    }

    kassert (vec[0] == 10);
    kassert (vec[1] == 9);
    kassert (vec[2] == 8);
    kassert (vec[3] == 7);
    kassert (vec[4] == 6);
    kassert (vec[5] == 5);
    kassert (vec[6] == 4);
    kassert (vec[7] == 3);
    kassert (vec[8] == 2);
    kassert (vec[9] == 1);
    kassert (vec[10] == 0);
  }

  {
    vector<int> first;
    vector<int> second;
    vector<int> third;

    first.assign (7, 100);

    second.assign (first.begin () + 1, first.end () - 1);

    int arr[] = {1776, 7, 4};
    third.assign (arr, arr + 3);

    kassert (first.size () == 7);
    kassert (first[0] == 100);
    kassert (first[1] == 100);
    kassert (first[2] == 100);
    kassert (first[3] == 100);
    kassert (first[4] == 100);
    kassert (first[5] == 100);
    kassert (first[6] == 100);

    kassert (second.size () == 5);
    kassert (second[0] == 100);
    kassert (second[1] == 100);
    kassert (second[2] == 100);
    kassert (second[3] == 100);
    kassert (second[4] == 100);

    kassert (third.size () == 3);
    kassert (third[0] == arr[0]);
    kassert (third[1] == arr[1]);
    kassert (third[2] == arr[2]);
  }

  {
    vector<int> vec;

    for (int i = 0; i < 10; ++i) {
      vec.push_back (i);
    }

    kassert (vec.size () == 10);
    kassert (vec[0] == 0);
    kassert (vec[1] == 1);
    kassert (vec[2] == 2);
    kassert (vec[3] == 3);
    kassert (vec[4] == 4);
    kassert (vec[5] == 5);
    kassert (vec[6] == 6);
    kassert (vec[7] == 7);
    kassert (vec[8] == 8);
    kassert (vec[9] == 9);
  }

  {
    vector<int> vec;
    int sum (0);

    vec.push_back (100);
    vec.push_back (200);
    vec.push_back (300);

    while (!vec.empty ()) {
      sum += vec.back ();
      vec.pop_back ();
    }

    kassert (sum == 600);
  }

  {
    vector<int> vec (3, 100);
    vector<int>::iterator it;

    it = vec.begin ();
    it = vec.insert (it, 200);

    vec.insert (it, 2, 300);

    it = vec.begin ();

    vector<int> vec2 (2, 400);
    vec.insert (it + 2, vec2.begin (), vec2.end ());

    int arr[] = { 501, 502, 503 };
    vec.insert (vec.begin (), arr, arr + 3);

    kassert (vec.size () == 11);
    kassert (vec[0] == arr[0]);
    kassert (vec[1] == arr[1]);
    kassert (vec[2] == arr[2]);
    kassert (vec[3] == 300);
    kassert (vec[4] == 300);
    kassert (vec[5] == 400);
    kassert (vec[6] == 400);
    kassert (vec[7] == 200);
    kassert (vec[8] == 100);
    kassert (vec[9] == 100);
    kassert (vec[10] == 100);
  }

  {
    vector<int> vec;

    for (int i = 1; i <= 10; i++) {
      vec.push_back (i);
    }

    vec.erase (vec.begin () + 5);
    
    vec.erase (vec.begin (), vec.begin () + 3);

    kassert (vec.size () == 6);
    kassert (vec[0] == 4);
    kassert (vec[1] == 5);
    kassert (vec[2] == 7);
    kassert (vec[3] == 8);
    kassert (vec[4] == 9);
    kassert (vec[5] == 10);
  }

  {
    vector<int> first (3, 100);
    vector<int> second (5, 200);

    first.swap (second);

    kassert (first.size () == 5);
    kassert (first[0] == 200);
    kassert (first[1] == 200);
    kassert (first[2] == 200);
    kassert (first[3] == 200);
    kassert (first[4] == 200);

    kassert (second.size () == 3);
    kassert (second[0] == 100);
    kassert (second[1] == 100);
    kassert (second[2] == 100);
  }

  {
    vector<int> vec;

    vec.push_back (100);
    vec.push_back (200);
    vec.push_back (300);

    kassert (vec.size () == 3);
    kassert (vec[0] == 100);
    kassert (vec[1] == 200);
    kassert (vec[2] == 300);

    vec.clear ();
    vec.push_back (1101);
    vec.push_back (2202);

    kassert (vec.size () == 2);
    kassert (vec[0] == 1101);
    kassert (vec[1] == 2202);
  }

  {
    vector<int> vec;

    int* p = vec.get_allocator ().allocate (5);

    for (int i = 0; i < 5; ++i) {
      p[i] = i;
    }

    kassert (p[0] == 0);
    kassert (p[1] == 1);
    kassert (p[2] == 2);
    kassert (p[3] == 3);
    kassert (p[4] == 4);

    vec.get_allocator ().deallocate (p, 5);
  }

  {
    vector<int> vec;

    for (int i = 0; i < 5; ++i) {
      vec.push_back (i);
    }

    *vec.begin () = 10;

    kassert (vec.size () == 5);
    vector<int>::const_iterator it = vec.begin ();
    kassert (*it++ == 10);
    kassert (*it++ == 1);
    kassert (*it++ == 2);
    kassert (*it++ == 3);
    kassert (*it++ == 4);
    kassert (it == vec.end ());
  }

  {
    vector<int> vec;
    int sum (0);

    for (int i = 1; i <= 10; ++i) {
      vec.insert (vec.end (), i);
    }

    for (vector<int>::const_iterator it = vec.begin ();
	 it != vec.end ();
	 ++it) {
      sum += *it;
    }

    kassert (sum == 55);
  }

  {
    vector<int> vec;

    for (int i = 0; i < 5; ++i) {
      vec.push_back (i);
    }

    *vec.rbegin () = 10;

    kassert (vec.size () == 5);
    vector<int>::const_reverse_iterator it = vec.rbegin ();
    kassert (*it++ == 10);
    kassert (*it++ == 3);
    kassert (*it++ == 2);
    kassert (*it++ == 1);
    kassert (*it++ == 0);
    kassert (it == vec.rend ());
  }

  {
    vector<int> first;
    vector<int> second (4, 100);
    vector<int> third (second.begin (), second.end ());
    vector<int> fourth (third);

    int arr[] = {16, 2, 77, 29};
    vector<int> fifth (arr, arr + 4);

    kassert (first.size () == 0);

    kassert (second.size () == 4);
    kassert (second[0] == 100);
    kassert (second[1] == 100);
    kassert (second[2] == 100);
    kassert (second[3] == 100);

    kassert (third.size () == 4);
    kassert (third[0] == 100);
    kassert (third[1] == 100);
    kassert (third[2] == 100);
    kassert (third[3] == 100);

    kassert (fourth.size () == 4);
    kassert (fourth[0] == 100);
    kassert (fourth[1] == 100);
    kassert (fourth[2] == 100);
    kassert (fourth[3] == 100);

    kassert (fifth.size () == 4);
    kassert (fifth[0] == 16);
    kassert (fifth[1] == 2);
    kassert (fifth[2] == 77);
    kassert (fifth[3] == 29);
  }

  {
    vector<int> first (3, 300);
    vector<int> second (5, 0);

    second = first;
    first = vector<int> ();

    kassert (first.size () == 0);

    kassert (second.size () == 3);
    kassert (second[0] == 300);
    kassert (second[1] == 300);
    kassert (second[2] == 300);
  }

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
    vm_area_t text;
    vm_area_initialize (&text,
			VM_AREA_TEXT,
			logical_address (&text_begin).align_down (PAGE_SIZE),
			logical_address (&text_end).align_up (PAGE_SIZE),
			SUPERVISOR);
    kassert (sys_automaton->insert_vm_area (&text) == 0);
    vm_area_t rodata;
    vm_area_initialize (&rodata,
			VM_AREA_DATA,
			logical_address (&rodata_begin).align_down (PAGE_SIZE),
			logical_address (&rodata_end).align_up (PAGE_SIZE),
			SUPERVISOR);
    kassert (sys_automaton->insert_vm_area (&rodata) == 0);
    vm_area_t data;
    vm_area_initialize (&data,
			VM_AREA_DATA,
			logical_address (&data_begin).align_down (PAGE_SIZE),
			logical_address (&data_end).align_up (PAGE_SIZE),
			SUPERVISOR);
    kassert (sys_automaton->insert_vm_area (&data) == 0);
    kassert (sys_automaton->insert_vm_area (boot_automaton.get_data_area ()) == 0);
  }
  {
    /* Add a stack. */
    vm_area_t stack;
    vm_area_initialize (&stack,
			VM_AREA_STACK,
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
