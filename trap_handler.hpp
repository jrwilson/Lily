#ifndef __trap_handler_hpp__
#define __trap_handler_hpp__

/*
  File
  ----
  trap_handler.hpp
  
  Description
  -----------
  Object for handling traps.

  Authors:
  Justin R. Wilson
*/

#include "interrupt_descriptor_table.hpp"

class trap_handler {
public:
  trap_handler (interrupt_descriptor_table& idt);
};

#endif /* __trap_handler_hpp__ */
