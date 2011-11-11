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

#include "automaton.hpp"

/* Does not return. */
void
system_automaton_initialize (logical_address placement_begin,
			     logical_address placement_end);

automaton_interface*
system_automaton_get_instance (void);

#endif /* __system_automaton_h__ */

