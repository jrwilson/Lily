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

#include "interrupt_descriptor_table.hpp"

class irq_handler {
private:
  uint8_t pic_master_mask_;
  uint8_t pic_slave_mask_;

public:
  irq_handler (interrupt_descriptor_table& idt);
};

#endif /* __irq_handler_hpp__ */
