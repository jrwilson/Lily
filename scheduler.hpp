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

#include "automaton.hpp"

void
scheduler_initialize (automaton_interface* automaton);

void
scheduler_set_switch_stack (logical_address switch_stack,
			    size_t switch_stack_size);

scheduler_context_t*
scheduler_allocate_context (list_allocator_t* list_allocator,
			    automaton* automaton);

automaton_interface*
scheduler_get_current_automaton (void);

void
schedule_action (automaton_interface* automaton,
		 void* action_entry_point,
		 parameter_t parameter);

void
finish_action (int output_status,
	       value_t value);

#endif /* __scheduler_h__ */
