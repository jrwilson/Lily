#ifndef __kassert_h__
#define __kassert_h__

/*
  File
  ----
  kassert.h
  
  Description
  -----------
  Macros for making assertions.

  Authors:
  Justin R. Wilson
*/

#include "kput.hpp"
#include "halt.hpp"
#include "quote.hpp"

#define kassert(expr) do { if (!(expr)) { kputs ("Assertion failed (" __FILE__ ":" quote(__LINE__) "): " #expr); halt (); } } while (0);

#endif /* __kassert_h__ */
