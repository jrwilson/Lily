#ifndef __aid_hpp__
#define __aid_hpp__

// Automaton Identifier (aid).

#include <stdint.h>
#include "static_assert.hpp"

typedef int32_t aid_t;

template <typename T>
inline aid_t
aid_cast (T v)
{
  STATIC_ASSERT (sizeof (T) == sizeof (aid_t));
  // Nasty C-style cast but it gets around static vs reinterpret casting issues.
  return (aid_t)v;
}

#endif /* __aid_hpp__ */
