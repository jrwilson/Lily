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

#include "automaton.h"

/* Does not return. */
void
system_automaton_initialize (void* placement_begin,
			     void* placement_end);

automaton_t*
system_automaton_get_instance (void);

#endif /* __system_automaton_h__ */

