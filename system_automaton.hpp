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
  run ();
  
  void
  page_fault (logical_address const& address,
	      uint32_t error);
  
  void
  schedule_action (void* action_entry_point,
		   parameter_t parameter);
  
  void
  finish_action (bool output_status,
		 value_t output_value);
  
  logical_address
  alloc (size_t size);
  
  void
  unknown_system_call (void);

  void
  bad_schedule (void);
}

#endif /* __system_automaton_hpp__ */
