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

namespace system_automaton {
  void
  run (logical_address placement_begin,
       logical_address placement_end);
  
  void
  page_fault (logical_address const& address,
	      uint32_t error,
	      registers* regs);
  
  void
  finish_action (size_t action_entry_point,
		 void* parameter,
		 void* buffer);
  
  logical_address
  alloc (size_t size);
  
  void
  unknown_system_call (void);

  void
  bad_schedule (void);

  void
  bad_message (void);
}

#endif /* __system_automaton_hpp__ */
