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

  static void
  schedule ();

  void
  init (automaton* p)
  {
    scheduler_.remove<init_traits> (&init, p);
    schedule ();
    scheduler_.finish<init_traits> (true);
  }

  static void
  schedule ()
  {
  }
}
