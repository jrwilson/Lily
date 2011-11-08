#ifndef __fifo_scheduler_h__
#define __fifo_scheduler_h__

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

#include "list_allocator.h"

typedef struct fifo_scheduler fifo_scheduler_t;

fifo_scheduler_t*
fifo_scheduler_allocate (list_allocator_t* list_allocator);

void
fifo_scheduler_add (fifo_scheduler_t*,
		    void* action_entry_point,
		    parameter_t parameter);

void
fifo_scheduler_remove (fifo_scheduler_t*,
		       void* action_entry_point,
		       parameter_t parameter);

void
fifo_scheduler_finish (fifo_scheduler_t*,
		       int output_status,
		       value_t output_value);

#define SCHEDULER_ADD fifo_scheduler_add
#define SCHEDULER_REMOVE fifo_scheduler_remove
#define SCHEDULER_FINISH fifo_scheduler_finish

#include "action_macros.h"

#endif /* __fifo_scheduler_h__ */
