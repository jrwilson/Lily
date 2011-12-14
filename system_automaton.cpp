/*
  File
  ----
  system_automaton.cpp
  
  Description
  -----------
  The system automaton.

  Authors:
  Justin R. Wilson
*/

#include "system_automaton.hpp"
#include "system_allocator.hpp"
#include "fifo_scheduler.hpp"

namespace system_automaton {
  
  typedef fifo_scheduler<system_allocator> scheduler_type;
  static scheduler_type scheduler_;

  struct schedule {
    void
    operator () () const;
  };
  
  static bool
  init_precondition (automaton*)
  {
    return true;
  }
  
  static void
  init_effect (automaton*)
  {
    // Do nothing.
  }
  
  void
  init (automaton* p)
  {
    output_action<init_traits> (p, scheduler_.remove<init_traits> (&init), init_precondition, init_effect, schedule (), scheduler_.finish ());
  }

  void
  schedule::operator() () const
  {
  }
}
