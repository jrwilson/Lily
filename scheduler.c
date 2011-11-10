/*
  File
  ----
  scheduler.c
  
  Description
  -----------
  Scheduling functions.

  Authors
  -------
  Justin R. Wilson
*/

#include "scheduler.h"
#include "kassert.h"
#include "idt.h"
#include "malloc.h"
#include "system_automaton.h"
#include "binding_manager.h"

typedef enum {
  SCHEDULED,
  NOT_SCHEDULED
} status_t;

struct scheduler_context {
  automaton_t* automaton;
  void* action_entry_point;
  parameter_t parameter;
  status_t status;
  scheduler_context_t* prev;
  scheduler_context_t* next;
};

typedef struct {
  void* switch_stack;
  size_t switch_stack_size;
  action_type_t action_type;
  automaton_t* automaton;
  void* action_entry_point;
  parameter_t parameter;
  input_action_t* input;
  value_t output_value;
} execution_context_t;

static scheduler_context_t* head = 0;
static scheduler_context_t* tail = 0;
/* TODO:  Need one per core. */
static execution_context_t exec_context;

void
scheduler_initialize (automaton_t* automaton)
{
  kassert (automaton != 0);

  exec_context.action_type = NO_ACTION;
  exec_context.automaton = automaton;
}

void
scheduler_set_switch_stack (void* switch_stack,
			    size_t switch_stack_size)
{
  kassert (switch_stack != 0);
  kassert (switch_stack >= (void*)KERNEL_VIRTUAL_BASE);
  kassert (switch_stack_size > 0);

  exec_context.switch_stack = switch_stack;
  exec_context.switch_stack_size = switch_stack_size;
}

scheduler_context_t*
scheduler_allocate_context (list_allocator_t* list_allocator,
			    automaton_t* automaton)
{
  kassert (list_allocator != 0);
  kassert (automaton != 0);
  scheduler_context_t* ptr = list_allocator_alloc (list_allocator, sizeof (scheduler_context_t));
  ptr->automaton = automaton;
  ptr->action_entry_point = 0;
  ptr->parameter = 0;
  ptr->status = NOT_SCHEDULED;
  ptr->prev = 0;
  ptr->next = 0;
  return ptr;
}

automaton_t*
scheduler_get_current_automaton (void)
{
  kassert (exec_context.automaton != 0);
  return exec_context.automaton;
}

void
schedule_action (automaton_t* automaton,
		 void* action_entry_point,
		 parameter_t parameter)
{
  kassert (automaton != 0);
  scheduler_context_t* ptr = automaton->scheduler_context;
  kassert (ptr != 0);

  ptr->action_entry_point = action_entry_point;
  ptr->parameter = parameter;

  if (ptr->status == NOT_SCHEDULED) {
    ptr->status = SCHEDULED;
    if (tail != 0) {
      /* Queue was not empty. */
      tail->next = ptr;
      ptr->prev = tail->next;
      tail = ptr;
    }
    else {
      /* Queue was empty. */
      head = ptr;
      tail = ptr;
    }
  }
}

static void
switch_to_next_action ()
{
  if (head != 0) {
    /* Load the execution context. */
    exec_context.automaton = head->automaton;
    exec_context.action_entry_point = head->action_entry_point;
    exec_context.parameter = head->parameter;

    /* Update the queue. */
    if (head != tail) {
      scheduler_context_t* tmp = head;
      head = tmp->next;
      head->prev = 0;
      tmp->status = NOT_SCHEDULED;
      tmp->next = 0;
      kassert (tmp->prev == 0);
    }
    else {
      scheduler_context_t* tmp = head;
      head = 0;
      tail = 0;
      tmp->status = NOT_SCHEDULED;
      kassert (tmp->prev == 0);
      kassert (tmp->next == 0);
    }

    /* Get the type to make sure its a local action. */
    exec_context.action_type = automaton_get_action_type (exec_context.automaton, exec_context.action_entry_point);
    switch (exec_context.action_type) {
    case OUTPUT:
      /* Get the inputs that are composed with the output. */
      exec_context.input = binding_manager_get_bound_inputs (exec_context.automaton, exec_context.action_entry_point, exec_context.parameter);
      /* Fall through. */
    case INTERNAL:
      /* Execute.  (This does not return). */
      automaton_execute (exec_context.switch_stack, exec_context.switch_stack_size, exec_context.automaton, exec_context.action_entry_point, exec_context.parameter, 0);
      break;
    case INPUT:
    case NO_ACTION:
      /* TODO:  Kill the automaton for scheduling a bad action. */
      kassert (0);
      /* Recur to get the next. */
      switch_to_next_action ();
      break;
    }
  }
  else {
    /* Out of actions.  Halt. */
    exec_context.action_type = NO_ACTION;
    enable_interrupts ();
    asm volatile ("hlt");
  }
}

void
finish_action (int output_status,
	       value_t output_value)
{
  switch (exec_context.action_type) {
  case INPUT:
    /* Move to the next input. */
    exec_context.input = exec_context.input->next;
    if (exec_context.input != 0) {
      /* Load the execution context. */
      exec_context.automaton = exec_context.input->automaton;
      exec_context.action_entry_point = exec_context.input->action_entry_point;
      exec_context.parameter = exec_context.input->parameter;
      /* Execute.  (This does not return). */
      automaton_execute (exec_context.switch_stack, exec_context.switch_stack_size, exec_context.automaton, exec_context.action_entry_point, exec_context.parameter, exec_context.output_value);
    }
    break;
  case OUTPUT:
    if (output_status && exec_context.input != 0) {
      /* The output executed and there are inputs. */
      exec_context.output_value = output_value;
      /* Load the execution context. */
      exec_context.action_type = INPUT;
      exec_context.automaton = exec_context.input->automaton;
      exec_context.action_entry_point = exec_context.input->action_entry_point;
      exec_context.parameter = exec_context.input->parameter;
      /* Execute.  (This does not return). */
      automaton_execute (exec_context.switch_stack, exec_context.switch_stack_size, exec_context.automaton, exec_context.action_entry_point, exec_context.parameter, exec_context.output_value);
    }
    break;
  case INTERNAL:
  case NO_ACTION:
    break;
  }

  switch_to_next_action ();
}
