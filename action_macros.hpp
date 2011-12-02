#ifndef __action_macros_hpp__
#define __action_macros_hpp__

/*
  File
  ----
  action_macros.hpp
  
  Description
  -----------
  Macros for declaring actions.

  Authors:
  Justin R. Wilson
*/

#include "quote.hpp"
#include "syscall.hpp"

/* Unvalued unparameterized input. */
#define UV_UP_INPUT(action_name)	\
  extern int action_name;		\
extern "C" void \
 action_name##_driver () \
{ \
  action_name##_effect ();	\
  action_name##_schedule (); \
  schedule_finish (0, 0);			\
} \
 asm (".global " quote(action_name) "\n" \
      quote(action_name) ":\n"	\
      "call " quote(action_name) "_driver");

/* Unvalued parameterized input. */
#define UV_P_INPUT(action_name, parameter_type)	\
  extern int action_name;		\
extern "C" void \
 action_name##_driver (parameter_t parameter) \
{ \
  action_name##_effect (reinterpret_cast<parameter_type> (parameter));		\
  action_name##_schedule (reinterpret_cast<parameter_type> (parameter)); \
  schedule_finish (0, 0);	\
} \
 asm (".global " quote(action_name) "\n" \
      quote(action_name) ":\n"	\
      "push %ecx\n" \
      "call " quote(action_name) "_driver");

/* Valued unparameterized input. */
#define V_UP_INPUT(action_name, value_type)		\
  extern int action_name; \
extern "C" void \
 action_name##_driver (value_t value) \
{ \
  action_name##_effect (reinterpret_cast<value_type> (value));		\
  action_name##_schedule (); \
  schedule_finish (0, 0);	\
} \
 asm (".global " quote(action_name) "\n" \
      quote(action_name) ":\n"	\
      "push %edx\n" \
      "call " quote(action_name) "_driver");

/* Valued parameterized input. */
#define V_P_INPUT(action_name, value_type, parameter_type)	\
  extern int action_name; \
extern "C" void \
 action_name##_driver (value_t value, parameter_t parameter)			\
{ \
  action_name##_effect (reinterpret_cast<value_type> (value), reinterpret_cast<parameter_type> (parameter));	\
  action_name##_schedule (reinterpret_cast<parameter_type> (parameter));				\
  schedule_finish (0, 0);			\
} \
 asm (".global " quote(action_name) "\n" \
      quote(action_name) ":\n"	\
      "push %ecx\n" \
      "push %edx\n" \
      "call " quote(action_name) "_driver");

/* Unvalued unparameterized output. */
#define UV_UP_OUTPUT(action_name)			\
  extern int action_name; \
extern "C" void \
action_name##_driver () \
{ \
  schedule_remove (&action_name, 0);	\
  bool status = action_name##_precondition ();	\
  if (status) { \
    action_name##_effect ();	\
  } \
  action_name##_schedule (); \
  schedule_finish (status, 0);	\
} \
 asm (".global " quote(action_name) "\n" \
      quote(action_name) ":\n"	\
      "call " quote(action_name) "_driver");

/* Unvalued parameterized output. */
#define UV_P_OUTPUT(action_name, parameter_type)	\
  extern int action_name; \
extern "C" void \
action_name##_driver (parameter_t parameter) \
{ \
  schedule_remove (&action_name, parameter);	\
  bool status = action_name##_precondition (reinterpret_cast<parameter_type> (parameter));	\
  if (status) { \
    action_name##_effect (reinterpret_cast<parameter_type> (parameter));		\
  } \
  action_name##_schedule (reinterpret_cast<parameter_type> (parameter));		\
  schedule_finish (status, 0);	\
} \
 asm (".global " quote(action_name) "\n" \
      quote(action_name) ":\n"	\
      "push %ecx\n" \
      "call " quote(action_name) "_driver");

/* Valued unparameterized output. */
#define V_UP_OUTPUT(action_name, value_type)		\
  extern int action_name; \
extern "C" void \
action_name##_driver () \
{ \
  schedule_remove (&action_name, 0);	\
  bool status = action_name##_precondition ();	\
  value_t value = 0;	       \
  if (status) { \
    value = reinterpret_cast<value_t> (action_name##_effect ());	\
  } \
  action_name##_schedule (); \
  schedule_finish (status, value);	\
} \
 asm (".global " quote(action_name) "\n" \
      quote(action_name) ":\n"	\
      "call " quote(action_name) "_driver");

/* Valued parameterized output. */
#define V_P_OUTPUT(action_name, value_type, parameter_type)	\
  extern int action_name; \
extern "C" void \
action_name##_driver (parameter_t parameter) \
{ \
  schedule_remove (&action_name, parameter);	\
  bool status = action_name##_precondition (reinterpret_cast<parameter_type> (parameter));	\
  value_t value = 0;	       \
  if (status) { \
    value = reinterpret_cast<value_t> (action_name##_effect (reinterpret_cast<parameter_type> (parameter))); \
  } \
  action_name##_schedule (reinterpret_cast<parameter_type> (parameter));		\
  schedule_finish (status, value);	\
} \
 asm (".global " quote(action_name) "\n" \
      quote(action_name) ":\n"	\
      "push %ecx\n" \
      "call " quote(action_name) "_driver");

#define UP_INTERNAL(action_name) \
extern int action_name; \
extern "C" void \
action_name##_driver () \
{ \
  schedule_remove (&action_name, 0); \
  if (action_name##_precondition ()) {	\
    action_name##_effect ();			\
  } \
  action_name##_schedule ();		\
  schedule_finish (0, 0); \
} \
asm (".global " quote(action_name) "\n"		\
     quote(action_name) ":\n"	\
     "call " quote(action_name) "_driver");

#define P_INTERNAL(action_name, parameter_type)	\
  extern int action_name; \
extern "C" void \
action_name##_driver (parameter_t parameter) \
{ \
  schedule_remove (&action_name, parameter);	\
  if (action_name##_precondition (reinterpret_cast<parameter_type> (parameter))) { \
    action_name##_effect (reinterpret_cast<parameter_type> (parameter));	\
  } \
  action_name##_schedule (reinterpret_cast<parameter_type> (parameter));	\
  schedule_finish (0, 0);	\
} \
asm (".global " quote(action_name) "\n"		\
     quote(action_name) ":\n"	\
      "push %ecx\n" \
     "call " quote(action_name) "_driver");

#endif /* __action_macros_hpp__ */
