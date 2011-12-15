#ifndef __rts_hpp__
#define __rts_hpp__

/*
  File
  ----
  rts.hpp
  
  Description
  -----------
  The run-time system (rts).
  Contains the system automaton, the set of bindings, and the scheduler.

  Authors:
  Justin R. Wilson
*/

#include "automaton.hpp"
#include "scheduler.hpp"

class rts {
public:
  static automaton* system_automaton;

  static void
  run ();

  static inline void
  page_fault (const void* address,
  	      uint32_t error,
  	      volatile registers* regs)
  {
    if (address < KERNEL_VIRTUAL_BASE) {
      // Use the automaton's memory map.
      scheduler_.current_automaton ()->page_fault (address, error, regs);
    }
    else {
      // Use our memory map.
      system_automaton->page_fault (address, error, regs);
    }
  }
  
  static inline void
  finish (const void* action_entry_point,
  	  aid_t parameter,
  	  bool output_status,
  	  const void* buffer)
  {
    if (action_entry_point != 0 || output_status) {
      automaton* current = scheduler_.current_automaton ();
      
      if (action_entry_point != 0) {
	// Check the action that was scheduled.
	automaton::const_action_iterator pos = current->action_find (action_entry_point);
	if (pos != current->action_end ()) {
	  scheduler_.schedule (caction (current, *pos, parameter));
	}
	else {
	  // TODO:  Automaton scheduled a bad action.
	  kassert (0);
	}
      }
      
      if (output_status) {
	// Check the buffer.
	size_t value_size = scheduler_.current_value_size ();
	if (value_size != 0 && !current->verify_span (buffer, value_size)) {
	  // TODO:  Automaton returned a bad buffer.
	  kassert (0);
	}
      }
    }

    scheduler_.finish (output_status, buffer);
  }

  static inline void*
  sbrk (ptrdiff_t size)
  {
    automaton* current_automaton = scheduler_.current_automaton ();
    // The system automaton should not use interrupts to acquire logical address space.
    kassert (current_automaton != system_automaton);

    return current_automaton->sbrk (size);
  }
  
  static inline void
  unknown_system_call (void)
  {
    // TODO
    kassert (0);
  }
 
private:
  typedef scheduler scheduler_type;
  static scheduler_type scheduler_;

  inline static void
  checked_schedule (automaton* a,
		    const void* aep,
		    aid_t p = 0)
  {
    automaton::const_action_iterator pos = a->action_find (aep);
    kassert (pos != a->action_end ());
    scheduler_.schedule (caction (a, *pos, p));
  }

  static void
  create_action_test ();
};

#endif /* __rts_hpp__ */
