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

typedef struct {
  /* aid_t aid; */
  unsigned int action_entry_point;
  unsigned int parameter;
} output_action_t;

/* Markers for the kernel. */
extern int text_begin;
extern int text_end;
extern int data_begin;
extern int data_end;
/* Stack is in data section. */
extern int stack_begin;
extern int stack_end;

static automaton_t* system_automaton = 0;
static list_allocator_t* system_allocator = 0;

static fifo_scheduler_t* scheduler = 0;
static unsigned int counter = 0;

static void
schedule ();

static int
init_precondition ()
{
  return scheduler == 0;
}

static void
init_effect ()
{
  kputs (__func__); kputs ("\n");
  scheduler = fifo_scheduler_allocate (system_allocator);
}

static void
init_schedule () {
  schedule ();
}

UP_INTERNAL (init);

static int
increment_precondition ()
{
  return counter < 10;
}

static void
increment_effect ()
{
  kputs ("counter = "); kputx32 (counter); kputs ("\n");
  ++counter;
}

static void
increment_schedule () {
  schedule ();
}

UP_INTERNAL (increment);

static void
schedule ()
{
  if (init_precondition ()) {
    SCHEDULER_ADD (scheduler, &init_entry, 0);
  }
  if (increment_precondition ()) {
    SCHEDULER_ADD (scheduler, &increment_entry, 0);
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
  automaton_initialize (&fake_automaton, RING0, vm_manager_page_directory_physical_address (), &stack_end, vm_manager_page_directory_logical_address (), SUPERVISOR);
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

  /* Allocate the real system automaton.
     The system automaton's ceiling is the start of the paging data structures. */
  automaton_t* sys_automaton = automaton_allocate (RING0, vm_manager_page_directory_physical_address (), &stack_end, vm_manager_page_directory_logical_address (), SUPERVISOR);

  /* Create a memory map for the system automatom. */
  vm_area_t* ptr;
  for (ptr = &text; ptr != 0; ptr = ptr->next) {
    kassert (automaton_insert_vm_area (sys_automaton, ptr) == 0);
  }

  /* Add the actions. */
  automaton_set_action_type (sys_automaton, &init_entry, INTERNAL);
  automaton_set_action_type (sys_automaton, &increment_entry, INTERNAL);

  /* Set the system automaton. */
  system_automaton = sys_automaton;

  /* Go! */
  schedule_action (system_automaton, &init_entry, 0);

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

list_allocator_t*
system_automaton_get_allocator (void)
{
  kassert (system_allocator != 0);
  return system_allocator;
}





















/* /\* The set of bindings. *\/ */
/* static hash_map_t* bindings = 0; */

/* /\* The hash map of bindings uses the output action as the key. *\/ */
/* static unsigned int */
/* output_action_hash_func (const void* x) */
/* { */
/*   const output_action_t* ptr = x; */
/*   return ptr->aid ^ ptr->action_entry_point ^ ptr->parameter; */
/* } */

/* static int */
/* output_action_compare_func (const void* x, */
/* 			    const void* y) */
/* { */
/*   const output_action_t* p1 = x; */
/*   const output_action_t* p2 = y; */
/*   if (p1->aid != p2->aid) { */
/*     return p1->aid - p2->aid; */
/*   } */
/*   else if (p1->action_entry_point != p2->action_entry_point) { */
/*     return p1->action_entry_point - p2->action_entry_point; */
/*   } */
/*   else { */
/*     return p1->parameter - p2->parameter; */
/*   } */
/* } */




/* #include "mm.h" */
/* #include "automata.h" */
/* #include "scheduler.h" */

/* extern int producer_init_entry; */
/* extern int producer_produce_entry; */
/* extern int consumer_init_entry; */
/* extern int consumer_consume_entry; */

  /* aid_t producer = create (RING0); */
  /* set_action_type (producer, (unsigned int)&producer_init_entry, INTERNAL); */
  /* set_action_type (producer, (unsigned int)&producer_produce_entry, OUTPUT); */

  /* aid_t consumer = create (RING0); */
  /* set_action_type (consumer, (unsigned int)&consumer_init_entry, INTERNAL); */
  /* set_action_type (consumer, (unsigned int)&consumer_consume_entry, INPUT); */

  /* bind (producer, (unsigned int)&producer_produce_entry, 0, consumer, (unsigned int)&consumer_consume_entry, 0); */

  /* initialize_scheduler (); */

  /* schedule_action (producer, (unsigned int)&producer_init_entry, 0); */
  /* schedule_action (consumer, (unsigned int)&consumer_init_entry, 0); */

  /* /\* Start the scheduler.  Doesn't return. *\/ */
  /* finish_action (0, 0); */

/* static output_action_t* */
/* allocate_output_action (aid_t aid, */
/* 			unsigned int action_entry_point, */
/* 			unsigned int parameter) */
/* { */
/*   output_action_t* ptr = kmalloc (sizeof (output_action_t)); */
/*   ptr->aid = aid; */
/*   ptr->action_entry_point = action_entry_point; */
/*   ptr->parameter = parameter; */
/*   return ptr; */
/* } */

/* static input_action_t* */
/* allocate_input_action (aid_t aid, */
/* 		       unsigned int action_entry_point, */
/* 		       unsigned int parameter) */
/* { */
/*   input_action_t* ptr = kmalloc (sizeof (input_action_t)); */
/*   ptr->aid = aid; */
/*   ptr->action_entry_point = action_entry_point; */
/*   ptr->parameter = parameter; */
/*   ptr->next = 0; */
/*   return ptr; */
/* } */

/* void */
/* bind (aid_t output_aid, */
/*       unsigned int output_action_entry_point, */
/*       unsigned int output_parameter, */
/*       aid_t input_aid, */
/*       unsigned int input_action_entry_point, */
/*       unsigned int input_parameter) */
/* { */
/*   kassert (bindings != 0); */
/*   /\* TODO:  All of the bind checks. *\/ */

/*   output_action_t output_action; */
/*   output_action.aid = output_aid; */
/*   output_action.action_entry_point = output_action_entry_point; */
/*   output_action.parameter = output_parameter; */

/*   if (!hash_map_contains (bindings, &output_action)) { */
/*     hash_map_insert (bindings, allocate_output_action (output_aid, output_action_entry_point, output_parameter), 0); */
/*   } */

/*   input_action_t* input_action = allocate_input_action (input_aid, input_action_entry_point, input_parameter); */
/*   input_action->next = hash_map_find (bindings, &output_action); */
/*   hash_map_erase (bindings, &output_action); */
/*   hash_map_insert (bindings, &output_action, input_action); */
/* } */

/* input_action_t* */
/* get_bound_inputs (aid_t output_aid, */
/* 		  unsigned int output_action_entry_point, */
/* 		  unsigned int output_parameter) */
/* { */
/*   kassert (bindings != 0); */

/*   output_action_t output_action; */
/*   output_action.aid = output_aid; */
/*   output_action.action_entry_point = output_action_entry_point; */
/*   output_action.parameter = output_parameter; */
/*   return hash_map_find (bindings, &output_action); */
/* } */

/* void */
/* bind (aid_t output_aid, */
/*       unsigned int output_action_entry_point, */
/*       unsigned int output_parameter, */
/*       aid_t input_aid, */
/*       unsigned int input_action_entry_point, */
/*       unsigned int input_parameter); */

/* /\* These functions allow the scheduler to query the set of automata and bindings. *\/ */

/* void* */
/* get_scheduler_context (aid_t aid); */

/* action_type_t */
/* get_action_type (aid_t aid, */
/* 		 unsigned int action_entry_point); */

/* void */
/* switch_to_automaton (aid_t aid, */
/* 		     unsigned int action_entry_point, */
/* 		     unsigned int parameter, */
/* 		     unsigned int input_value); */

/* input_action_t* */
/* get_bound_inputs (aid_t output_aid, */
/* 		  unsigned int output_action_entry_point, */
/* 		  unsigned int output_parameter); */
