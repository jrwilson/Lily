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
  run (const void* placement_begin,
       const void* placement_end);
  
  void
  page_fault (const void* address,
	      uint32_t error,
	      registers* regs);
  
  void
  finish_action (const void* action_entry_point,
		 aid_t parameter,
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
