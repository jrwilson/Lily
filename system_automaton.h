#ifndef __system_automaton_h__
#define __system_automaton_h__

/*
  File
  ----
  system_automaton.h
  
  Description
  -----------
  The system automaton.

  Authors:
  Justin R. Wilson
*/

#include "syscall_def.h"

typedef struct automaton automaton_t;

typedef enum {
  NO_ACTION,
  INPUT,
  OUTPUT,
  INTERNAL
} action_type_t;

typedef struct input_action input_action_t;
struct input_action {
  //aid_t aid;
  unsigned int action_entry_point;
  unsigned int parameter;
  input_action_t* next;
};

/* Does not return. */
void
system_automaton_initialize (void* placement_begin,
			     void* placement_end);

void*
automaton_allocate (automaton_t* automaton,
		    unsigned int size,
		    syserror_t* error);

#endif /* __system_automaton_h__ */

