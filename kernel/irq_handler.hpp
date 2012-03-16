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

extern volatile unsigned int mono_seconds;
extern volatile unsigned int mono_nanoseconds;

namespace irq_handler {
  static const int IRQ_BASE = 0;
  static const int IRQ_LIMIT = 16;

  static const int PIT_IRQ = 0;
  static const int CASCADE_IRQ = 2;

  void
  install ();

  void
  subscribe (int irq,
	     const caction& action);

  void
  unsubscribe (int irq,
	       const caction& action);

  void
  process_interrupts ();

  inline void
  wait_for_interrupt (void)
  {
    asm volatile ("hlt");
  }

  inline void
  getmonotime (mono_time_t* t)
  {
    asm volatile ("cli");
    t->seconds = mono_seconds;
    t->nanoseconds = mono_nanoseconds;
    asm volatile ("sti");
  }
};

#endif /* __irq_handler_hpp__ */
