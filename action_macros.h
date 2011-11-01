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

#define V_UP_INPUT(name) \
extern unsigned int name##_entry; \
 asm (".global " quote(name) "_entry\n" \
      quote(name) "_entry:\n"	\
      "push %edx\n" \
      "call " quote(name) "_driver"); \
void \
 name##_driver (unsigned int value) \
{ \
  name##_effect (value);	\
  name##_schedule (); \
  SCHEDULER_FINISH (scheduler, 0, 0);		\
}

#define V_UP_OUTPUT(name) \
extern unsigned int name##_entry; \
 asm (".global " quote(name) "_entry\n" \
      quote(name) "_entry:\n"	\
      "call " quote(name) "_driver"); \
void \
name##_driver () \
{ \
  SCHEDULER_REMOVE (scheduler, (unsigned int)&name##_entry, 0); \
  int status = name##_precondition (); \
  unsigned int value = 0; \
  if (status) { \
    value = name##_effect ();	\
  } \
  name##_schedule (); \
  SCHEDULER_FINISH (scheduler, status, value);	\
}

#define UP_INTERNAL(name) \
extern unsigned int name##_entry; \
 asm (".global " quote(name) "_entry\n" \
      quote(name) "_entry:\n"	\
      "call " quote(name) "_driver"); \
void \
name##_driver () \
{ \
  SCHEDULER_REMOVE (scheduler, (unsigned int)&name##_entry, 0); \
  if (name##_precondition ()) { \
    name##_effect (); \
  } \
  name##_schedule (); \
  SCHEDULER_FINISH (scheduler, 0, 0);		\
}

#endif /* __action_macros_h__ */
