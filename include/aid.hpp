#ifndef __aid_hpp__
#define __aid_hpp__

// Automaton Identifier (aid).

#include <stdint.h>

typedef int32_t aid_t;

template <typename T>
inline aid_t
aid_cast (T v)
{
  static_assert (sizeof (T) == sizeof (aid_t), "sizeof (parameter type) must be equal to sizeof (aid)");
  // Nasty C-style cast but it gets around static vs reinterpret casting issues.
  return (aid_t)v;
}

#endif /* __aid_hpp__ */
