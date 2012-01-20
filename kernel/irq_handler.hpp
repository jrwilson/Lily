#ifndef __irq_handler_hpp__
#define __irq_handler_hpp__

/*
  File
  ----
  irq_handler.hpp
  
  Description
  -----------
  Object for handling irqs.

  Authors:
  Justin R. Wilson
*/

#include "integer_types.hpp"

class irq_handler {
public:
  static void
  install ();

private:
  static uint8_t pic_master_mask_;
  static uint8_t pic_slave_mask_;
};

#endif /* __irq_handler_hpp__ */
