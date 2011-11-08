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
#include "types.h"
#include "automaton.h"
#include "list_allocator.h"

typedef struct input_action input_action_t;
struct input_action {
  unsigned int action_entry_point;
  unsigned int parameter;
  input_action_t* next;
};

/* Does not return. */
void
system_automaton_initialize (void* placement_begin,
			     void* placement_end);

automaton_t*
system_automaton_get_instance (void);

list_allocator_t*
system_automaton_get_allocator (void);

#endif /* __system_automaton_h__ */

