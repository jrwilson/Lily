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

#include "types.hpp"

class irq_handler {
private:
  static uint8_t pic_master_mask_;
  static uint8_t pic_slave_mask_;

public:
  static void
  install ();
};

#endif /* __irq_handler_hpp__ */
