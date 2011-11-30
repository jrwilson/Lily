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
#define UV_UP_INPUT(name, scheduler)	\
  extern int name;		\
  static void name##_effect (void); \
  static void name##_schedule (void);		\
extern "C" void \
 name##_driver () \
{ \
  name##_effect ();	\
  name##_schedule (); \
  (scheduler).finish (0, 0);			\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "call " quote(name) "_driver");

/* Unvalued parameterized input. */
#define UV_P_INPUT(name, parameter_type, scheduler)	\
  extern int name;		\
  static void name##_effect (parameter_type); \
  static void name##_schedule (parameter_type);		\
extern "C" void \
 name##_driver (parameter_t parameter) \
{ \
  name##_effect ((parameter_type)parameter);		\
  name##_schedule ((parameter_type)parameter); \
  (scheduler).finish (0, 0);	\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "push %ecx\n" \
      "call " quote(name) "_driver");

/* Valued unparameterized input. */
#define V_UP_INPUT(name, value_type, scheduler)		\
  extern int name; \
  static void name##_effect (value_type); \
  static void name##_schedule (void);		\
extern "C" void \
 name##_driver (value_t value) \
{ \
  name##_effect ((value_type)value);		\
  name##_schedule (); \
  (scheduler).finish (0, 0);	\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "push %edx\n" \
      "call " quote(name) "_driver");

/* Valued parameterized input. */
#define V_P_INPUT(name, value_type, parameter_type, scheduler)	\
  extern int name; \
  static void name##_effect (value_type, parameter_type);	\
  static void name##_schedule (parameter_type);		\
extern "C" void \
 name##_driver (value_t value, parameter_t parameter)			\
{ \
  name##_effect ((value_type)value, (parameter_type)parameter);	\
  name##_schedule ((parameter_type)parameter);				\
  (scheduler).finish (0, 0);			\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "push %ecx\n" \
      "push %edx\n" \
      "call " quote(name) "_driver");

/* Unvalued unparameterized output. */
#define UV_UP_OUTPUT(name, scheduler)			\
  extern int name; \
  static bool name##_precondition (void); \
  static void name##_effect (void); \
  static void name##_schedule (void);		\
extern "C" void \
name##_driver () \
{ \
  (scheduler).remove (&name, 0);	\
  bool status = name##_precondition ();	\
  if (status) { \
    name##_effect ();	\
  } \
  name##_schedule (); \
  (scheduler).finish (status, 0);	\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "call " quote(name) "_driver");

/* Unvalued parameterized output. */
#define UV_P_OUTPUT(name, parameter_type, scheduler)	\
  extern int name; \
  static bool name##_precondition (parameter_type); \
  static void name##_effect (parameter_type); \
  static void name##_schedule (parameter_type);		\
extern "C" void \
name##_driver (parameter_t parameter) \
{ \
  (scheduler).remove (&name, parameter);	\
  bool status = name##_precondition ((parameter_type)parameter);	\
  if (status) { \
    name##_effect ((parameter_type)parameter);		\
  } \
  name##_schedule ((parameter_type)parameter);		\
  (scheduler).finish (status, 0);	\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "push %ecx\n" \
      "call " quote(name) "_driver");

/* Valued unparameterized output. */
#define V_UP_OUTPUT(name, value_type, scheduler)		\
  extern int name; \
  static bool name##_precondition (void); \
  static value_type name##_effect (void); \
  static void name##_schedule (void); \
extern "C" void \
name##_driver () \
{ \
  (scheduler).remove (&name, 0);	\
  bool status = name##_precondition ();	\
  value_t value = 0;	       \
  if (status) { \
    value = (value_t)name##_effect ();		\
  } \
  name##_schedule (); \
  (scheduler).finish (status, value);	\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "call " quote(name) "_driver");

/* Valued parameterized output. */
#define V_P_OUTPUT(name, value_type, parameter_type, scheduler)	\
  extern int name; \
  static bool name##_precondition (parameter_type); \
  static value_type name##_effect (parameter_type); \
  static void name##_schedule (parameter_type); \
extern "C" void \
name##_driver (parameter_t parameter) \
{ \
  (scheduler).remove (&name, parameter);	\
  bool status = name##_precondition ((parameter_type)parameter);	\
  value_t value = 0;	       \
  if (status) { \
    value = (value_t)name##_effect ((parameter_type)parameter);	\
  } \
  name##_schedule ((parameter_type)parameter);		\
  (scheduler).finish (status, value);	\
} \
 asm (".global " quote(name) "\n" \
      quote(name) ":\n"	\
      "push %ecx\n" \
      "call " quote(name) "_driver");

#define UP_INTERNAL(class_name, action_name)	\
extern "C" void \
action_name##_driver () \
{ \
  class_name::schedule_remove (&action_name, 0); \
  if (class_name::action_name##_precondition ()) {	\
    class_name::action_name##_effect ();			\
  } \
  class_name::action_name##_schedule ();		\
  class_name::schedule_finish (0, 0); \
} \
asm (".global " quote(action_name) "\n"		\
     quote(action_name) ":\n"	\
     "call " quote(action_name) "_driver");

#define P_INTERNAL(name, parameter_type, scheduler)	\
  extern int name; \
  static bool name##_precondition (parameter_type); \
  static void name##_effect (parameter_type); \
  static void name##_schedule (parameter_type); \
extern "C" void \
name##_driver (parameter_t parameter) \
{ \
  (scheduler).remove (&name, parameter);	\
  if (name##_precondition ((parameter_type)parameter)) {	  \
    name##_effect ((parameter_type)parameter);				  \
  } \
  name##_schedule ((parameter_type)parameter);		\
  (scheduler).finish (0, 0);	\
} \
asm (".global " quote(name) "\n"		\
     quote(name) ":\n"	\
      "push %ecx\n" \
     "call " quote(name) "_driver");

#endif /* __action_macros_hpp__ */
