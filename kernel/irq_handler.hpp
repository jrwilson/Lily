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
#include "action.hpp"
#include "utility.hpp"

namespace irq_handler {

  void
  install ();

  void
  subscribe (int irq,
	     const caction& action);

  void
  unsubscribe (int irq,
	       const caction& action);

  static const int MIN_IRQ = 0;
  static const int MAX_IRQ = 15;

};

#endif /* __irq_handler_hpp__ */
