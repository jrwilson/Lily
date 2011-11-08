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

/* typedef enum { */
/*   SCHEDULED, */
/*   NOT_SCHEDULED */
/* } status_t; */

/* struct scheduler_context { */
/*   aid_t aid; */
/*   unsigned int action_entry_point; */
/*   unsigned int parameter; */
/*   status_t status; */
/*   scheduler_context_t* prev; */
/*   scheduler_context_t* next; */
/* }; */

/* typedef struct { */
/*   action_type_t action_type; */
/*   aid_t aid; */
/*   unsigned int action_entry_point; */
/*   unsigned int parameter; */
/*   input_action_t* input; */
/*   unsigned int output_value; */
/* } execution_context_t; */

static automaton_t* current_automaton = 0;
/* static scheduler_context_t* head = 0; */
/* static scheduler_context_t* tail = 0; */
/* /\* TODO:  Need one per core. *\/ */
/* static execution_context_t exec_context; */

void
scheduler_initialize (automaton_t* automaton)
{
  kassert (current_automaton == 0);
  kassert (automaton != 0);

  current_automaton = automaton;
}


/* scheduler_context_t* */
/* allocate_scheduler_context (aid_t aid) */
/* { */
/*   kassert (0); */
  /* scheduler_context_t* ptr = kmalloc (sizeof (scheduler_context_t)); */
  /* ptr->aid = aid; */
  /* ptr->action_entry_point = 0; */
  /* ptr->parameter = 0; */
  /* ptr->status = NOT_SCHEDULED; */
  /* ptr->prev = 0; */
  /* ptr->next = 0; */
  /* return ptr; */
/* } */

automaton_t*
scheduler_get_current_automaton (void)
{
  kassert (current_automaton != 0);
  return current_automaton;
}

void
schedule_action (unsigned int action_entry_point,
		 unsigned int parameter)
{
/*   kassert (0); */
  /* scheduler_context_t* ptr = get_scheduler_context (aid); */
  /* kassert (ptr != 0); */

  /* ptr->action_entry_point = action_entry_point; */
  /* ptr->parameter = parameter; */

  /* if (ptr->status == NOT_SCHEDULED) { */
  /*   ptr->status = SCHEDULED; */
  /*   if (tail != 0) { */
  /*     /\* Queue was not empty. *\/ */
  /*     tail->next = ptr; */
  /*     ptr->prev = tail->next; */
  /*     tail = ptr; */
  /*   } */
  /*   else { */
  /*     /\* Queue was empty. *\/ */
  /*     head = ptr; */
  /*     tail = ptr; */
  /*   } */
  /* } */
}

static void
switch_to_next_action ()
{
  kassert (0);
  /* if (head != 0) { */
  /*   /\* Load the execution context. *\/ */
  /*   exec_context.aid = head->aid; */
  /*   exec_context.action_entry_point = head->action_entry_point; */
  /*   exec_context.parameter = head->parameter; */

  /*   /\* Update the queue. *\/ */
  /*   if (head != tail) { */
  /*     scheduler_context_t* tmp = head; */
  /*     head = tmp->next; */
  /*     head->prev = 0; */
  /*     tmp->status = NOT_SCHEDULED; */
  /*     tmp->next = 0; */
  /*     kassert (tmp->prev == 0); */
  /*   } */
  /*   else { */
  /*     scheduler_context_t* tmp = head; */
  /*     head = 0; */
  /*     tail = 0; */
  /*     tmp->status = NOT_SCHEDULED; */
  /*     kassert (tmp->prev == 0); */
  /*     kassert (tmp->next == 0); */
  /*   } */

  /*   /\* Get the type to make sure its a local action. *\/ */
  /*   exec_context.action_type = get_action_type (exec_context.aid, exec_context.action_entry_point); */
  /*   switch (exec_context.action_type) { */
  /*   case OUTPUT: */
  /*     /\* Get the inputs that are composed with the output. *\/ */
  /*     exec_context.input = get_bound_inputs (exec_context.aid, exec_context.action_entry_point, exec_context.parameter); */
  /*     /\* Fall through. *\/ */
  /*   case INTERNAL: */
  /*     /\* Execute.  (This does not return). *\/ */
  /*     switch_to_automaton (exec_context.aid, exec_context.action_entry_point, exec_context.parameter, 0); */
  /*     break; */
  /*   case INPUT: */
  /*   case NO_ACTION: */
  /*     /\* TODO:  Kill the automaton for scheduling a bad action. *\/ */
  /*     kputs ("Bad schedule\n"); */
  /*     /\* Recur to get the next. *\/ */
  /*     switch_to_next_action (); */
  /*     break; */
  /*   } */
  /* } */
  /* else { */
  /*   /\* Out of actions.  Halt. *\/ */
  /*   exec_context.action_type = NO_ACTION; */
  /*   enable_interrupts (); */
  /*   asm volatile ("hlt"); */
  /* } */
}

void
finish_action (int output_status,
	       unsigned int output_value)
{
  kassert (0);
  /* switch (exec_context.action_type) { */
  /* case INPUT: */
  /*   /\* Move to the next input. *\/ */
  /*   exec_context.input = exec_context.input->next; */
  /*   if (exec_context.input != 0) { */
  /*     /\* Load the execution context. *\/ */
  /*     exec_context.aid = exec_context.input->aid; */
  /*     exec_context.action_entry_point = exec_context.input->action_entry_point; */
  /*     exec_context.parameter = exec_context.input->parameter; */
  /*     /\* Execute.  (This does not return). *\/ */
  /*     switch_to_automaton (exec_context.aid, exec_context.action_entry_point, exec_context.parameter, exec_context.output_value); */
  /*   } */
  /*   break; */
  /* case OUTPUT: */
  /*   if (output_status && exec_context.input != 0) { */
  /*     /\* The output executed and there are inputs. *\/ */
  /*     exec_context.output_value = output_value; */
  /*     /\* Load the execution context. *\/ */
  /*     exec_context.action_type = INPUT; */
  /*     exec_context.aid = exec_context.input->aid; */
  /*     exec_context.action_entry_point = exec_context.input->action_entry_point; */
  /*     exec_context.parameter = exec_context.input->parameter; */
  /*     /\* Execute.  (This does not return). *\/ */
  /*     switch_to_automaton (exec_context.aid, exec_context.action_entry_point, exec_context.parameter, exec_context.output_value); */
  /*   } */
  /*   break; */
  /* case INTERNAL: */
  /* case NO_ACTION: */
  /*   break; */
  /* } */

  /* switch_to_next_action (); */
}
