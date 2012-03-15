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

namespace irq_handler {
  static const int IRQ_BASE = 0;
  static const int IRQ_LIMIT = 16;

  static const int PIT_IRQ = 0;
  static const int CASCADE_IRQ = 2;

  void
  install ();

  void
  enable_irq (int irq);

  void
  disable_irq (int irq);

  uint16_t
  get_irqs (void);
};

#endif /* __irq_handler_hpp__ */
