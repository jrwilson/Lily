#ifndef __quote_h__
#define __quote_h__

/*
  File
  ----
  quote.h
  
  Description
  -----------
  Macro for quoting.

  Authors:
  Justin R. Wilson
*/

#define quote_(expr) #expr
#define quote(expr) quote_(expr)

#endif /* __quote_h__ */
