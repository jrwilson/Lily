#ifndef __exception_handler_hpp__
#define __exception_handler_hpp__

/*
  File
  ----
  exception_handler.hpp
  
  Description
  -----------
  Object for handling exceptions.

  Authors:
  Justin R. Wilson
*/

#include "interrupt_descriptor_table.hpp"

namespace exception_handler {
  void install (interrupt_descriptor_table& idt);

}

#endif /* __exception_handler_hpp__ */
