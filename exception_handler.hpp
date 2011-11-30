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

class exception_handler {
public:
  exception_handler (interrupt_descriptor_table& idt);
};

#endif /* __exception_handler_hpp__ */
