#ifndef __static_assert_hpp__
#define __static_assert_hpp__

/*
  File
  ----
  static_assert.hpp
  
  Description
  -----------
  Macro replacement for static assert.

  Authors:
  Justin R. Wilson
*/

#include "comma.hpp"

template <bool> struct static_assert_helper;

template <>
struct static_assert_helper<true> { };

#define STATIC_ASSERT(p) ( { static_assert_helper<p> (); })

#endif /* __static_assert_hpp__ */
