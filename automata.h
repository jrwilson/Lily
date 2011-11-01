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

typedef enum {
  NO_ACTION,
  INPUT,
  OUTPUT,
  INTERNAL
} action_type_t;

typedef struct input_action input_action_t;
struct input_action {
  aid_t aid;
  unsigned int action_entry_point;
  unsigned int parameter;
  input_action_t* next;
};

void
initialize_automata (void);

/* These functions are for manually creating automata and bindings.
   They do not check for errors.
   You have been warned.
*/

aid_t
create (privilege_t privilege);

void
set_action_type (aid_t aid,
		 unsigned int action_entry_point,
		 action_type_t action_type);

void
bind (aid_t output_aid,
      unsigned int output_action_entry_point,
      unsigned int output_parameter,
      aid_t input_aid,
      unsigned int input_action_entry_point,
      unsigned int input_parameter);

/* These functions allow the scheduler to query the set of automata and bindings. */

void*
get_scheduler_context (aid_t aid);

action_type_t
get_action_type (aid_t aid,
		 unsigned int action_entry_point);

void
switch_to_automaton (aid_t aid,
		     unsigned int action_entry_point,
		     unsigned int parameter,
		     unsigned int input_value);

input_action_t*
get_bound_inputs (aid_t output_aid,
		  unsigned int output_action_entry_point,
		  unsigned int output_parameter);

#endif /* __automata_h__ */
