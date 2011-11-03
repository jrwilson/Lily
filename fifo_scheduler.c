/*
  File
  ----
  fifo_scheduler.h
  
  Description
  -----------
  A fifo scheduler for automata.

  Authors:
  Justin R. Wilson
*/

#include "fifo_scheduler.h"
#include "kassert.h"
#include "mm.h"
#include "syscall.h"

typedef struct fifo_scheduler_entry fifo_scheduler_entry_t;

struct fifo_scheduler_entry {
  fifo_scheduler_entry_t* next;
  unsigned int action_entry_point;
  unsigned int parameter;
};

struct fifo_scheduler {
  fifo_scheduler_entry_t* ready;
  fifo_scheduler_entry_t* free;
};

fifo_scheduler_t*
allocate_fifo_scheduler ()
{
  fifo_scheduler_t* ptr = kmalloc (sizeof (fifo_scheduler_t));
  ptr->ready = 0;
  ptr->free = 0;
  return ptr;
}

static fifo_scheduler_entry_t*
allocate_scheduler_entry (fifo_scheduler_t* ptr,
			  unsigned int action_entry_point,
			  unsigned int parameter)
{
  fifo_scheduler_entry_t* p;
  if (ptr->free != 0) {
    p = ptr->free;
    ptr->free = p->next;
  }
  else {
    p = kmalloc (sizeof (fifo_scheduler_entry_t));
  }
  p->next = 0;
  p->action_entry_point = action_entry_point;
  p->parameter = parameter;
  return p;
}

void
fifo_scheduler_add (fifo_scheduler_t* ptr,
		    unsigned int action_entry_point,
		    unsigned int parameter)
{
  kassert (ptr != 0);
  fifo_scheduler_entry_t** p;
  for (p = &ptr->ready; (*p) != 0 && !((*p)->action_entry_point == action_entry_point && (*p)->parameter == parameter); p = &((*p)->next)) ;;
  if (*p == 0) {
    *p = allocate_scheduler_entry (ptr, action_entry_point, parameter);
  }
}

static void
free_scheduler_entry (fifo_scheduler_t* ptr,
		      fifo_scheduler_entry_t* p)
{
  p->next = ptr->free;
  ptr->free = p;
}

void
fifo_scheduler_remove (fifo_scheduler_t* ptr,
		       unsigned int action_entry_point,
		       unsigned int parameter)
{
  /* Easier to write action macros if this is conditional. */
  if (ptr != 0) {
    fifo_scheduler_entry_t** p;
    for (p = &ptr->ready; (*p) != 0 && !((*p)->action_entry_point == action_entry_point && (*p)->parameter == parameter); p = &((*p)->next)) ;;
    if (*p != 0) {
      fifo_scheduler_entry_t* tmp = *p;
      *p = tmp->next;
      free_scheduler_entry (ptr, tmp);
    }
  }
}

void
fifo_scheduler_finish (fifo_scheduler_t* ptr,
		       int output_status,
		       unsigned int output_value)
{
  kassert (ptr != 0);
  if (ptr->ready != 0) {
    /* Schedule. */
    sys_schedule (ptr->ready->action_entry_point, ptr->ready->parameter, output_status, output_value);
  }
  else {
    /* Don't schedule. */
    sys_finish (output_status, output_value);
  }
}
