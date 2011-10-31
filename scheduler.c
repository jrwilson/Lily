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
#include "memory.h"
#include "interrupt.h"

typedef enum {
  SCHEDULED,
  NOT_SCHEDULED
} status_t;

struct scheduler_context {
  aid_t aid;
  unsigned int action_entry_point;
  unsigned int parameter;
  status_t status;
  scheduler_context_t* prev;
  scheduler_context_t* next;
};

static scheduler_context_t* head = 0;
static scheduler_context_t* tail = 0;
/* TODO:  Need one per core. */
static scheduler_context_t* current = 0;

scheduler_context_t*
allocate_scheduler_context (aid_t aid)
{
  scheduler_context_t* ptr = kmalloc (sizeof (scheduler_context_t));
  ptr->aid = aid;
  ptr->action_entry_point = 0;
  ptr->parameter = 0;
  ptr->status = NOT_SCHEDULED;
  ptr->prev = 0;
  ptr->next = 0;
  return ptr;
}

static void
schedule_context (scheduler_context_t* ptr,
		  unsigned int action_entry_point,
		  unsigned int parameter)
{
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

void
schedule_aid (aid_t aid,
	      unsigned int action_entry_point,
	      unsigned int parameter)
{
  schedule_context (get_scheduler_context (aid), action_entry_point, parameter);
}

void
schedule (unsigned int action_entry_point,
	  unsigned int parameter)
{
  schedule_context (current, action_entry_point, parameter);
}

void
finish_action ()
{
  if (head != 0) {
    if (head != tail) {
      current = head;
      head = current->next;
      head->prev = 0;
      current->status = NOT_SCHEDULED;
      current->next = 0;
      kassert (current->prev == 0);
    }
    else {
      current = head;
      head = 0;
      tail = 0;
      current->status = NOT_SCHEDULED;
      kassert (current->prev == 0);
      kassert (current->next == 0);
    }

    switch_to_automaton (current->aid, current->action_entry_point, current->parameter);
  }
  else {
    enable_interrupts ();
    /* No work.  Halt. */
    asm volatile ("hlt");
  }
}
