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

#include "kput.h"
#include "halt.h"

#define quote_(expr) #expr
#define quote(expr) quote_(expr)
#define kassert(expr) do { if (!(expr)) { kputs ("Assertion failed (" __FILE__ ":" quote(__LINE__) "): " #expr); halt (); } } while (0);

#endif /* __kassert_h__ */



