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

void
initialize_scheduler ();

scheduler_context_t*
allocate_scheduler_context (aid_t aid);

aid_t
get_current_aid (void);

void
schedule_action (aid_t aid,
		 unsigned int action_entry_point,
		 unsigned int parameter);

void
finish_action (int output_status,
	       unsigned int value);

#endif /* __scheduler_h__ */
