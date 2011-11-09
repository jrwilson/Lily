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

#include "automaton.h"

void
scheduler_initialize (automaton_t* automaton);

scheduler_context_t*
scheduler_allocate_context (list_allocator_t* list_allocator,
			    automaton_t* automaton);

automaton_t*
scheduler_get_current_automaton (void);

void
schedule_action (automaton_t* automaton,
		 void* action_entry_point,
		 parameter_t parameter);

void
finish_action (int output_status,
	       value_t value);

#endif /* __scheduler_h__ */
