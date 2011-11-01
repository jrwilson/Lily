#ifndef __scheduler_h__
#define __scheduler_h__

/*
  File
  ----
  scheduler.h
  
  Description
  -----------
  Declarations for scheduling.

  Authors:
  Justin R. Wilson
*/

#include "automata.h"

typedef struct scheduler_context scheduler_context_t;

scheduler_context_t*
allocate_scheduler_context (aid_t aid);

void
schedule_aid (aid_t aid,
	      unsigned int action_entry_point,
	      unsigned int parameter);

void
schedule_action (unsigned int action_entry_point,
		 unsigned int parameter);

void
finish_action (void);

#endif /* __scheduler_h__ */
