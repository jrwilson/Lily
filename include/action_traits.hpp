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

// The absence of a type.
struct null_type { };

// Parameters.
struct no_parameter_tag { };
struct parameter_tag { };
struct auto_parameter_tag { };

enum parameter_mode_t {
  NO_PARAMETER = 0x0001,
  PARAMETER = 0x0002,
  AUTO_PARAMETER = 0x0004,
};

struct no_parameter {
  typedef no_parameter_tag parameter_category;
  typedef null_type parameter_type;
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

// Buffer values.
struct no_buffer_value_tag { };
struct buffer_value_tag { };

enum buffer_value_mode_t {
  NO_BUFFER_VALUE = 0x0008,
  BUFFER_VALUE = 0x0010,
};

struct no_buffer_value {
  typedef no_buffer_value_tag buffer_value_category;
  static const buffer_value_mode_t buffer_value_mode = NO_BUFFER_VALUE;
  typedef null_type buffer_value_type;
};

template <class Value>
struct buffer_value {
  typedef buffer_value_tag buffer_value_category;
  static const buffer_value_mode_t buffer_value_mode = BUFFER_VALUE;
  typedef Value buffer_value_type;
};

// Size of the temporary buffer used to store the values produced by output actions.
const size_t MAX_COPY_VALUE_SIZE = 512;

// Copy values.
struct no_copy_value_tag { };
struct copy_value_tag { };

enum copy_value_mode_t {
  NO_COPY_VALUE = 0x0020,
  COPY_VALUE = 0x0040,
};

struct no_copy_value {
  typedef no_copy_value_tag copy_value_category;
  static const copy_value_mode_t copy_value_mode = NO_COPY_VALUE;
  typedef null_type copy_value_type;
  static const size_t copy_value_size = 0;
};

template <class Value>
struct copy_value {
  typedef copy_value_tag copy_value_category;
  static const copy_value_mode_t copy_value_mode = COPY_VALUE;
  typedef Value copy_value_type;
  static const size_t copy_value_size = sizeof (Value);
};

// Actions.
struct input_action_tag { };
struct output_action_tag { };
struct internal_action_tag { };

enum action_type_t {
  INPUT = 0x0080,
  OUTPUT = 0x0100,
  INTERNAL = 0x0200,
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

// Put it all together.
struct np_nb_nc_input_action_traits : public no_parameter, public no_buffer_value, public no_copy_value, public input_action { };

template <typename CopyValue>
struct np_nb_c_input_action_traits : public no_parameter, public no_buffer_value, public copy_value<CopyValue>, public input_action { };

template <typename BufferValue>
struct np_b_nc_input_action_traits : public no_parameter, public buffer_value<BufferValue>, public no_copy_value, public input_action { };

template <typename BufferValue, typename CopyValue>
struct np_b_c_input_action_traits : public no_parameter, public buffer_value<BufferValue>, public copy_value<CopyValue>, public input_action { };

template <typename Parameter>
struct p_nb_nc_input_action_traits : public parameter<Parameter>, public no_buffer_value, public no_copy_value, public input_action { };

template <typename Parameter, typename CopyValue>
struct p_nb_c_input_action_traits : public parameter<Parameter>, public no_buffer_value, public copy_value<CopyValue>, public input_action { };

template <typename Parameter, typename BufferValue>
struct p_b_nc_input_action_traits : public parameter<Parameter>, public buffer_value<BufferValue>, public no_copy_value, public input_action { };

template <typename Parameter, typename BufferValue, typename CopyValue>
struct p_b_c_input_action_traits : public parameter<Parameter>, public buffer_value<BufferValue>, public copy_value<CopyValue>, public input_action { };

struct ap_nb_nc_input_action_traits : public auto_parameter, public no_buffer_value, public no_copy_value, public input_action { };

template <typename CopyValue>
struct ap_nb_c_input_action_traits : public auto_parameter, public no_buffer_value, public copy_value<CopyValue>, public input_action { };

template <typename BufferValue>
struct ap_b_nc_input_action_traits : public auto_parameter, public buffer_value<BufferValue>, public no_copy_value, public input_action { };

template <typename BufferValue, typename CopyValue>
struct ap_b_c_input_action_traits : public auto_parameter, public buffer_value<BufferValue>, public copy_value<CopyValue>, public input_action { };

struct np_nb_nc_output_action_traits : public no_parameter, public no_buffer_value, public no_copy_value, public output_action { };

template <typename CopyValue>
struct np_nb_c_output_action_traits : public no_parameter, public no_buffer_value, public copy_value<CopyValue>, public output_action { };

template <typename BufferValue>
struct np_b_nc_output_action_traits : public no_parameter, public buffer_value<BufferValue>, public no_copy_value, public output_action { };

template <typename BufferValue, typename CopyValue>
struct np_b_c_output_action_traits : public no_parameter, public buffer_value<BufferValue>, public copy_value<CopyValue>, public output_action { };

template <typename Parameter>
struct p_nb_nc_output_action_traits : public parameter<Parameter>, public no_buffer_value, public no_copy_value, public output_action { };

template <typename Parameter, typename CopyValue>
struct p_nb_c_output_action_traits : public parameter<Parameter>, public no_buffer_value, public copy_value<CopyValue>, public output_action { };

template <typename Parameter, typename BufferValue>
struct p_b_nc_output_action_traits : public parameter<Parameter>, public buffer_value<BufferValue>, public no_copy_value, public output_action { };

template <typename Parameter, typename BufferValue, typename CopyValue>
struct p_b_c_output_action_traits : public parameter<Parameter>, public buffer_value<BufferValue>, public copy_value<CopyValue>, public output_action { };

struct ap_nb_nc_output_action_traits : public auto_parameter, public no_buffer_value, public no_copy_value, public output_action { };

template <typename CopyValue>
struct ap_nb_c_output_action_traits : public auto_parameter, public no_buffer_value, public copy_value<CopyValue>, public output_action { };

template <typename BufferValue>
struct ap_b_nc_output_action_traits : public auto_parameter, public buffer_value<BufferValue>, public no_copy_value, public output_action { };

template <typename BufferValue, typename CopyValue>
struct ap_b_c_output_action_traits : public auto_parameter, public buffer_value<BufferValue>, public copy_value<CopyValue>, public output_action { };

struct np_internal_action_traits : public no_parameter, public no_buffer_value, public no_copy_value, public internal_action { };

template <class Parameter>
struct p_internal_action_traits : public parameter<Parameter>, public no_buffer_value, public no_copy_value, public internal_action  { };

// These structs encode the schema of action traits.
template <bool>
struct bool_dispatch : public std::false_type { };

template <>
struct bool_dispatch<true> : public std::true_type { };

template <class T>
struct is_parameter : public bool_dispatch<
  ((std::is_same<typename T::parameter_category, no_parameter_tag>::value && std::is_same<typename T::parameter_type, null_type>::value && T::parameter_mode == NO_PARAMETER) ||
   (std::is_same<typename T::parameter_category, parameter_tag>::value && sizeof (typename T::parameter_type) == sizeof (aid_t) && T::parameter_mode == PARAMETER) ||
   (std::is_same<typename T::parameter_category, auto_parameter_tag>::value && std::is_same<typename T::parameter_type, aid_t>::value && T::parameter_mode == AUTO_PARAMETER))
  > { };

template <class T>
struct is_buffer_value : public bool_dispatch<
  ((std::is_same<typename T::buffer_value_category, no_buffer_value_tag>::value &&
    T::buffer_value_mode == NO_BUFFER_VALUE &&
    std::is_same<typename T::buffer_value_type, null_type>::value) ||
   (std::is_same<typename T::buffer_value_category, buffer_value_tag>::value &&
    T::buffer_value_mode == BUFFER_VALUE && 
    !std::is_same<typename T::buffer_value_type, null_type>::value))
  > { };

template <class T>
struct is_copy_value : public bool_dispatch<
  ((std::is_same<typename T::copy_value_category, no_copy_value_tag>::value &&
    T::copy_value_mode == NO_COPY_VALUE &&
    std::is_same<typename T::copy_value_type, null_type>::value &&
    T::copy_value_size == 0) ||
   (std::is_same<typename T::copy_value_category, copy_value_tag>::value &&
    T::copy_value_mode == COPY_VALUE &&
    !std::is_same<typename T::copy_value_type, null_type>::value &&
    sizeof (typename T::copy_value_type) <= MAX_COPY_VALUE_SIZE && sizeof (typename T::copy_value_type) == T::copy_value_size))
    > { };

template <class T>
struct is_input_action : public bool_dispatch<
  is_parameter<T>::value &&
  is_buffer_value<T>::value &&
  is_copy_value<T>::value &&
  std::is_same<typename T::action_category, input_action_tag>::value && T::action_type == INPUT> { };

template <class T>
struct is_output_action : public bool_dispatch<
  is_parameter<T>::value &&
  is_buffer_value<T>::value &&
  is_copy_value<T>::value &&
  std::is_same<typename T::action_category, output_action_tag>::value && T::action_type == OUTPUT> { };

template <class T>
struct is_internal_action : public bool_dispatch<
  is_parameter<T>::value &&
  std::is_same<typename T::action_category, internal_action_tag>::value && T::action_type == INTERNAL> { };

template <class T>
struct is_local_action : public bool_dispatch<is_output_action<T>::value || is_internal_action<T>::value> { };

template <class T>
struct is_action : public bool_dispatch<is_input_action<T>::value || is_output_action<T>::value || is_internal_action<T>::value> { };

#ifdef NOTE
#include <iostream>

template <class T>
struct type_formatter;

template <>
struct type_formatter<void>
{
  static const char*
  get_format ()
  {
    return "void";
  }
};

template <>
struct type_formatter<null_type>
{
  static const char*
  get_format ()
  {
    return "";
  }
};

template <class T>
class action_printer {
public:
  action_printer (const char* func_name,
		  const char* export_name)
  {
    static_assert (is_action<T>::value, "Type is not an action");

    std::cout << ".pushsection .action_info, \"\", @note\n"
	      << ".balign 4\n"
	      << ".long 1f - 0f\n"	// Length of the author string.
	      << ".long 3f - 2f\n"	// Length of the description.
	      << ".long 0\n"		// The type.  0 => action descriptor
	      << "0: .asciz \"lily\"\n"	// The author.
	      << "1: .balign 4\n"
	      << "2:\n"			// The description.
	      << ".long " << func_name << "\n"	// Action entry point.
	      << ".long " << (T::action_type | T::parameter_mode | T::buffer_value_mode | T::copy_value_mode) << "\n"
	      << ".long " << T::copy_value_size << "\n" 	// Copy value size.
	      << ".asciz \"" << export_name << "\"\n"		// Export the action using this name.
	      << ".asciz \"" << type_formatter<typename T::buffer_value_type>::get_format () << "\"\n"	// A string describing the buffer value.
	      << ".asciz \"" << type_formatter<typename T::copy_value_type>::get_format () << "\"\n"	// A string describing the copy value.
      	      << "3: .balign 4\n"
	      << ".popsection\n" << std::endl;
  }
};
#define ACTION(traits, func_name, share_name) action_printer<traits> share_name_ (#func_name, #share_name);
#else
#define ACTION(traits, func_name, share_name)
#endif

#endif /* __action_traits_hpp__ */
