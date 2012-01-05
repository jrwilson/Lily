#ifndef __action_traits_hpp__
#define __action_traits_hpp__

/*
  File
  ----
  action_traits.hpp
  
  Description
  -----------
  Types for describing actions.
  Algorithms for executing actions.

  Authors:
  Justin R. Wilson
*/

#include <type_traits>
#include "aid.hpp"
#include "bid.hpp"
#include <stddef.h>

typedef aid_t no_param_t;

// Actions.
struct input_action_tag { };
struct output_action_tag { };
struct internal_action_tag { };

#define M_INPUT 0
#define M_OUTPUT 1
#define M_INTERNAL 2

enum action_type_t {
  INPUT = M_INPUT,
  OUTPUT = M_OUTPUT,
  INTERNAL = M_INTERNAL,
};

struct input_action {
  typedef input_action_tag action_category;
  static const action_type_t action_type = INPUT;
};

struct output_action {
  typedef output_action_tag action_category;
  static const action_type_t action_type = OUTPUT;
};

struct internal_action {
  typedef internal_action_tag action_category;
  static const action_type_t action_type = INTERNAL;
};

// Parameters.
struct no_parameter_tag { };
struct parameter_tag { };
struct auto_parameter_tag { };

#define M_NO_PARAMETER 0
#define M_PARAMETER 1
#define M_AUTO_PARAMETER 2

enum parameter_mode_t {
  NO_PARAMETER = M_NO_PARAMETER,
  PARAMETER = M_PARAMETER,
  AUTO_PARAMETER = M_AUTO_PARAMETER,
};

struct no_parameter {
  typedef no_parameter_tag parameter_category;
  typedef no_param_t parameter_type;
  static const parameter_mode_t parameter_mode = NO_PARAMETER;
};

template <class Parameter>
struct parameter {
  typedef parameter_tag parameter_category;
  typedef Parameter parameter_type;
  static const parameter_mode_t parameter_mode = PARAMETER;
};

struct auto_parameter {
  typedef auto_parameter_tag parameter_category;
  typedef aid_t parameter_type;
  static const parameter_mode_t parameter_mode = AUTO_PARAMETER;
};

// Size of the temporary buffer used to store the values produced by output actions.
const size_t MAX_COPY_VALUE_SIZE = 512;

template <typename Action,
	  typename Parameter = no_parameter,
	  typename CopyValueType = void,
	  typename BufferValueType = void>
struct action_traits : public Action, public Parameter {
  typedef CopyValueType copy_value_type;
  typedef BufferValueType buffer_value_type;
};

// These structs encode the schema of action traits.
template <bool>
struct bool_dispatch : public std::false_type { };

template <>
struct bool_dispatch<true> : public std::true_type { };

template <class T>
struct is_parameter : public bool_dispatch<
  ((std::is_same<typename T::parameter_category, no_parameter_tag>::value && std::is_same<typename T::parameter_type, no_param_t>::value && T::parameter_mode == NO_PARAMETER) ||
   (std::is_same<typename T::parameter_category, parameter_tag>::value && sizeof (typename T::parameter_type) == sizeof (aid_t) && T::parameter_mode == PARAMETER) ||
   (std::is_same<typename T::parameter_category, auto_parameter_tag>::value && std::is_same<typename T::parameter_type, aid_t>::value && T::parameter_mode == AUTO_PARAMETER))
  > { };

template <class T>
struct copy_value_size
{
  static const size_t value = sizeof (T);
};

template <>
struct copy_value_size<void>
{
  static const size_t value = 0;
};

template <class T>
struct is_copy_value : public bool_dispatch<
  std::is_same<typename T::copy_value_type, void>::value ||
  copy_value_size<typename T::copy_value_type>::value <= MAX_COPY_VALUE_SIZE
					  > { };

template <class T>
struct is_input_action : public bool_dispatch<
  std::is_same<typename T::action_category, input_action_tag>::value && T::action_type == INPUT &&
  is_parameter<T>::value &&
  is_copy_value<T>::value
  > { };

template <class T>
struct is_output_action : public bool_dispatch<
  std::is_same<typename T::action_category, output_action_tag>::value && T::action_type == OUTPUT &&
  is_parameter<T>::value &&
  is_copy_value<T>::value
  > { };

template <class T>
struct is_internal_action : public bool_dispatch<
  std::is_same<typename T::action_category, internal_action_tag>::value && T::action_type == INTERNAL &&
  is_parameter<T>::value &&
  T::parameter_mode != AUTO_PARAMETER
  > { };

template <class T>
struct is_local_action : public bool_dispatch<is_output_action<T>::value || is_internal_action<T>::value> { };

template <class T>
struct is_action : public bool_dispatch<is_input_action<T>::value || is_output_action<T>::value || is_internal_action<T>::value> { };

template <typename T,
	  typename C>
struct fp_typedef1;

template <typename T>
struct fp_typedef1 <T, input_action_tag>
{
  typedef void (*function_pointer_type) (typename T::parameter_type, typename T::copy_value_type*, size_t, bid_t, size_t);
};

template <typename T>
struct fp_typedef1 <T, output_action_tag>
{
  typedef void (*function_pointer_type) (typename T::parameter_type);
};

template <typename T>
struct fp_typedef1 <T, internal_action_tag>
{
  typedef void (*function_pointer_type) (typename T::parameter_type);
};

template <typename T>
struct fp_typedef : public fp_typedef1<T, typename T::action_category> { };

#include "quote.hpp"

// A macro for embedding the action information.
#define ACTION_DESCRIPTOR(base, func_name)				\
  static_assert (is_action<base##_TRAITS>::value, "traits is not an action trait"); \
  static_assert (base##_ACTION == base##_TRAITS::action_type, quote (base) "_ACTION does not agree with traits"); \
  static_assert (base##_PARAMETER == base##_TRAITS::parameter_mode, quote (base) "_PARAMETER does not agree with traits"); \
  class base##_function_pointer_check { \
    int y;						     \
    base##_function_pointer_check ()					\
    {								\
      fp_typedef<base##_TRAITS>::function_pointer_type x = func_name;	\
      if (x != 0) {						\
	y = 3;							\
      }								\
    }								\
  };								\
  asm (".pushsection .action_info, \"\", @note\n"		   \
  ".balign 4\n"							   \
  ".long 1f - 0f\n"		/* Length of the author string. */ \
  ".long 3f - 2f\n"		/* Length of the description. */ \
  ".long 0\n"			/* The type.  0 => action descriptor. */ \
  "0: .asciz \"lily\"\n"	/* The author of the note. */ \
  "1: .balign 4\n" \
  "2:\n"			/* The description of the note. */ \
  ".long 5f - 4f\n"		/* Length of the action name. */ \
  ".long 3f - 5f\n"		/* Length of the action description. */ \
  ".long 0\n"			/* Action description comparison method. 0 => string compare. */ \
  ".long " quote (base##_ACTION) "\n"	/* Action type. */ \
  ".long " #func_name "\n"		/* Action entry point. */		\
  ".long " quote (base##_PARAMETER) "\n"	/* Parameter mode. */ \
  "4: .asciz \"" base##_NAME "\"\n"		/* The name of this action. */ \
  "5: .asciz \"" base##_DESCRIPTION "\"\n"	/* The description of this action. */\
  "3: .balign 4\n" \
       ".popsection\n");

#endif /* __action_traits_hpp__ */
