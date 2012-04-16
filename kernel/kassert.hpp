#ifndef __kassert_hpp__
#define __kassert_hpp__

/*
  File
  ----
  kassert.hpp
  
  Description
  -----------
  Macros for making assertions.

  Authors:
  Justin R. Wilson
*/

#include "kout.hpp"
#include "halt.hpp"
#include "lily/quote.h"

#define kassert(expr) do { if (!(expr)) { kout << "Assertion failed (" __FILE__ ":" quote(__LINE__) "): " #expr; halt (); } } while (0);

#define kpanic(msg) do { kout << "Kernel panic (" __FILE__ ":" quote(__LINE__) "): " << msg << endl; halt (); } while (0);

#endif /* __kassert_hpp__ */
