#ifndef QUOTE_H
#define QUOTE_H

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

#endif /* QUOTE_H */
