#ifndef __type_traits_hpp__
#define __type_traits_hpp__

/*
  File
  ----
  type_traits.hpp
  
  Description
  -----------
  Type traits.

  Authors:
  Justin R. Wilson
*/

template <class T, T V>
struct integral_constant {
  static const T value = V;
  typedef T value_type;
  typedef integral_constant<T, V> type;
};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template <class T> struct is_void : public false_type { };
template <> struct is_void<void> : public true_type { };

template <class T> struct is_integral : public false_type { };
template <> struct is_integral<bool> : public true_type { };
template <> struct is_integral<char> : public true_type { };
template <> struct is_integral<signed char> : public true_type { };
template <> struct is_integral<unsigned char> : public true_type { };
template <> struct is_integral<short> : public true_type { };
template <> struct is_integral<int> : public true_type { };
template <> struct is_integral<long> : public true_type { };
template <> struct is_integral<long long> : public true_type { };
template <> struct is_integral<unsigned short> : public true_type { };
template <> struct is_integral<unsigned int> : public true_type { };
template <> struct is_integral<unsigned long> : public true_type { };
template <> struct is_integral<unsigned long long> : public true_type { };

#endif /* __type_traits_hpp__ */
