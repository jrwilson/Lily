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

/* Unvalued unparameterized input. */
#define UV_UP_INPUT(name) \
  extern int name;		\
  static void name##_effect (void); \
  static void name##_schedule (void);		\
extern "C" void \
 name##_driver () \
{ \
  name##_effect ();	\
  name##_schedule (); \
  SCHEDULER_FINISH (scheduler, 0, 0);		\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "call " quote(name) "_driver");

/* Unvalued parameterized input. */
#define UV_P_INPUT(name, parameter_type)	\
  extern int name;		\
  static void name##_effect (parameter_type); \
  static void name##_schedule (parameter_type);		\
extern "C" void \
 name##_driver (parameter_t parameter) \
{ \
  name##_effect ((parameter_type)parameter);		\
  name##_schedule ((parameter_type)parameter); \
  SCHEDULER_FINISH (scheduler, 0, 0);		\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "push %ecx\n" \
      "call " quote(name) "_driver");

/* Valued unparameterized input. */
#define V_UP_INPUT(name, value_type)			\
  extern int name; \
  static void name##_effect (value_type); \
  static void name##_schedule (void);		\
extern "C" void \
 name##_driver (value_t value) \
{ \
  name##_effect ((value_type)value);		\
  name##_schedule (); \
  SCHEDULER_FINISH (scheduler, 0, 0);		\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "push %edx\n" \
      "call " quote(name) "_driver");

/* Valued parameterized input. */
#define V_P_INPUT(name, value_type, parameter_type)		\
  extern int name; \
  static void name##_effect (value_type, parameter_type);	\
  static void name##_schedule (parameter_type);		\
extern "C" void \
 name##_driver (value_t value, parameter_t parameter)			\
{ \
  name##_effect ((value_type)value, (parameter_type)parameter);	\
  name##_schedule ((parameter_type)parameter);				\
  SCHEDULER_FINISH (scheduler, 0, 0);		\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "push %ecx\n" \
      "push %edx\n" \
      "call " quote(name) "_driver");

/* Unvalued unparameterized output. */
#define UV_UP_OUTPUT(name) \
  extern int name; \
  static int name##_precondition (void); \
  static void name##_effect (void); \
  static void name##_schedule (void);		\
extern "C" void \
name##_driver () \
{ \
  SCHEDULER_REMOVE (scheduler, &name, 0); \
  int status = name##_precondition (); \
  if (status) { \
    name##_effect ();	\
  } \
  name##_schedule (); \
  SCHEDULER_FINISH (scheduler, status, 0);	\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "call " quote(name) "_driver");

/* Unvalued parameterized output. */
#define UV_P_OUTPUT(name, parameter_type)			\
  extern int name; \
  static int name##_precondition (parameter_type); \
  static void name##_effect (parameter_type); \
  static void name##_schedule (parameter_type);		\
extern "C" void \
name##_driver (parameter_t parameter) \
{ \
  SCHEDULER_REMOVE (scheduler, &name, parameter); \
  int status = name##_precondition ((parameter_type)parameter);	\
  if (status) { \
    name##_effect ((parameter_type)parameter);		\
  } \
  name##_schedule ((parameter_type)parameter);		\
  SCHEDULER_FINISH (scheduler, status, 0);	\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "push %ecx\n" \
      "call " quote(name) "_driver");

/* Valued unparameterized output. */
#define V_UP_OUTPUT(name, value_type)			\
  extern int name; \
  static int name##_precondition (void); \
  static value_type name##_effect (void); \
  static void name##_schedule (void); \
extern "C" void \
name##_driver () \
{ \
  SCHEDULER_REMOVE (scheduler, &name, 0); \
  int status = name##_precondition (); \
  value_t value = 0;	       \
  if (status) { \
    value = (value_t)name##_effect ();		\
  } \
  name##_schedule (); \
  SCHEDULER_FINISH (scheduler, status, value);	\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "call " quote(name) "_driver");

/* Valued parameterized output. */
#define V_P_OUTPUT(name, value_type, parameter_type)		\
  extern int name; \
  static int name##_precondition (parameter_type); \
  static value_type name##_effect (parameter_type); \
  static void name##_schedule (parameter_type); \
extern "C" void \
name##_driver (parameter_t parameter) \
{ \
  SCHEDULER_REMOVE (scheduler, &name, parameter); \
  int status = name##_precondition ((parameter_type)parameter);	\
  value_t value = 0;	       \
  if (status) { \
    value = (value_t)name##_effect ((parameter_type)parameter);	\
  } \
  name##_schedule ((parameter_type)parameter);		\
  SCHEDULER_FINISH (scheduler, status, value);	\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "push %ecx\n" \
      "call " quote(name) "_driver");

#define UP_INTERNAL(name) \
  extern int name; \
  static int name##_precondition (void); \
  static void name##_effect (void); \
  static void name##_schedule (void); \
extern "C" void \
name##_driver () \
{ \
  SCHEDULER_REMOVE (scheduler, &name, 0); \
  if (name##_precondition ()) { \
    name##_effect (); \
  } \
  name##_schedule (); \
  SCHEDULER_FINISH (scheduler, 0, 0);		\
} \
asm (".global " quote(name) "\n"		\
     quote(name) ":\n"	\
     "call " quote(name) "_driver");

#define P_INTERNAL(name, parameter_type)			\
  extern int name; \
  static int name##_precondition (parameter_type); \
  static void name##_effect (parameter_type); \
  static void name##_schedule (parameter_type); \
extern "C" void \
name##_driver (parameter_t parameter) \
{ \
  SCHEDULER_REMOVE (scheduler, &name, parameter); \
  if (name##_precondition ((parameter_type)parameter)) {	  \
    name##_effect ((parameter_type)parameter);				  \
  } \
  name##_schedule ((parameter_type)parameter);		\
  SCHEDULER_FINISH (scheduler, 0, 0);		\
} \
asm (".global " quote(name) "\n"		\
     quote(name) ":\n"	\
      "push %ecx\n" \
     "call " quote(name) "_driver");

#endif /* __action_macros_h__ */
