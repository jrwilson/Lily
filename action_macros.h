#ifndef __action_macros_h__
#define __action_macros_h__

/*
  File
  ----
  action_macros.h
  
  Description
  -----------
  Macros for declaring actions.
  Assumes that SCHEDULER_REMOVE and SCHEDULER_FINISH are defined and that the variable named "scheduler" is the scheduler.

  Authors:
  Justin R. Wilson
*/

#include "quote.h"

#define UP_INTERNAL(name) \
extern unsigned int name##_entry; \
 asm (".global " quote(name) "_entry\n" \
      quote(name) "_entry:\n"	\
      "push %eax\n" \
      "call " quote(name) "_driver"); \
void \
name##_driver (unsigned int parameter) \
{ \
  SCHEDULER_REMOVE (scheduler, (unsigned int)&name##_entry, parameter); \
  if (name##_precondition ()) { \
    name##_effect (); \
  } \
  name##_schedule (); \
  SCHEDULER_FINISH (scheduler); \
}

#endif /* __action_macros_h__ */
