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
#include "system_automaton.hpp"

extern "C" void trap_dispatch (registers);

class trap_handler {
private:
  static trap_handler* instance_;

  system_automaton& system_automaton_;

  void
  process_interrupt (registers&);

public:
  trap_handler (interrupt_descriptor_table& idt,
		system_automaton& s_a);

  friend void trap_dispatch (registers);
};

#endif /* __trap_handler_hpp__ */
