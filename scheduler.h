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

#include "system_automaton.h"

typedef struct scheduler_context scheduler_context_t;

void
scheduler_initialize (automaton_t* automaton);

/* scheduler_context_t* */
/* allocate_scheduler_context (aid_t aid); */

automaton_t*
scheduler_get_current_automaton (void);

void
schedule_action (unsigned int action_entry_point,
		 unsigned int parameter);

void
finish_action (int output_status,
	       unsigned int value);

#endif /* __scheduler_h__ */
