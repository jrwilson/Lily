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
#include "syscall.h"

typedef struct fifo_scheduler_entry fifo_scheduler_entry_t;

struct fifo_scheduler_entry {
  fifo_scheduler_entry_t* next;
  void* action_entry_point;
  parameter_t parameter;
};

struct fifo_scheduler {
  list_allocator_t* list_allocator;
  fifo_scheduler_entry_t* ready;
  fifo_scheduler_entry_t* free;
};

fifo_scheduler_t*
fifo_scheduler_allocate (list_allocator_t* list_allocator)
{
  fifo_scheduler_t* ptr = list_allocator_alloc (list_allocator, sizeof (fifo_scheduler_t));
  ptr->list_allocator = list_allocator;
  ptr->ready = 0;
  ptr->free = 0;
  return ptr;
}

static fifo_scheduler_entry_t*
allocate_scheduler_entry (fifo_scheduler_t* ptr,
			  void* action_entry_point,
			  parameter_t parameter)
{
  fifo_scheduler_entry_t* p;
  if (ptr->free != 0) {
    p = ptr->free;
    ptr->free = p->next;
  }
  else {
    p = list_allocator_alloc (ptr->list_allocator, sizeof (fifo_scheduler_entry_t));
  }
  p->next = 0;
  p->action_entry_point = action_entry_point;
  p->parameter = parameter;
  return p;
}

void
fifo_scheduler_add (fifo_scheduler_t* ptr,
		    void* action_entry_point,
		    parameter_t parameter)
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
		       void* action_entry_point,
		       parameter_t parameter)
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
		       value_t output_value)
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
