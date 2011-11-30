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
#include "system_automaton.hpp"

extern "C" void exception_dispatch (registers);

class exception_handler {
private:
  static exception_handler* instance_;

  system_automaton& system_automaton_;

  void
  process_interrupt (registers&);

public:
  exception_handler (interrupt_descriptor_table& idt,
		     system_automaton& s_a);

  friend void exception_dispatch (registers);
};

#endif /* __exception_handler_hpp__ */
