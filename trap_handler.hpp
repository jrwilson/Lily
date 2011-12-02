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

namespace trap_handler {

  void
  install (interrupt_descriptor_table& idt);

}

#endif /* __trap_handler_hpp__ */
