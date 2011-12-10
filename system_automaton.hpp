#ifndef __system_automaton_hpp__
#define __system_automaton_hpp__

/*
  File
  ----
  system_automaton.hpp
  
  Description
  -----------
  Declarations for the system automaton.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"
#include "interrupt_descriptor_table.hpp"

class automaton;

namespace system_automaton {

  extern automaton* system_automaton;

  void
  run ();
  
  void
  page_fault (const void* address,
	      uint32_t error,
	      registers* regs);
  
  void
  finish_action (const void* action_entry_point,
		 aid_t parameter,
		 bool output_status,
		 const void* buffer);
  
  void*
  alloc (size_t size);
  
  void
  unknown_system_call (void);

  void
  bad_schedule (void);

  void
  bad_value (void);
}

#endif /* __system_automaton_hpp__ */
