#ifndef __automata_h__
#define __automata_h__

/*
  File
  ----
  automata.h
  
  Description
  -----------
  Declarations for functions for managing the set of automata.

  Authors:
  Justin R. Wilson
*/

#include "descriptor.h"

typedef int aid_t;

void
initialize_automata (void);

aid_t
create_automaton (privilege_t privilege);

void*
get_scheduler_context (aid_t aid);

void
switch_to_automaton (aid_t aid,
		     unsigned int action_entry_point,
		     unsigned int parameter);

#endif /* __automata_h__ */
