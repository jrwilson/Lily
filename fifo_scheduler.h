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

typedef struct fifo_scheduler fifo_scheduler_t;

fifo_scheduler_t*
allocate_fifo_scheduler (void);

void
fifo_scheduler_add (fifo_scheduler_t*,
		    unsigned int action_entry_point,
		    unsigned int parameter);

void
fifo_scheduler_remove (fifo_scheduler_t*,
		       unsigned int action_entry_point,
		       unsigned int parameter);

void
fifo_scheduler_finish (fifo_scheduler_t*);

#define SCHEDULER_ADD fifo_scheduler_add
#define SCHEDULER_REMOVE fifo_scheduler_remove
#define SCHEDULER_FINISH fifo_scheduler_finish

#include "action_macros.h"

#endif /* __fifo_scheduler_h__ */
