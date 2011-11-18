/*
  File
  ----
  fifo_scheduler.h
  
  Description
  -----------
  A fifo scheduler for automata.

  Authors:
  Justin R. Wilson
*/

#include "fifo_scheduler.hpp"
#include "syscall.hpp"
#include <algorithm>

void
fifo_scheduler::add (void* action_entry_point,
		     parameter_t parameter)
{
  entry e (action_entry_point, parameter);
  if (set_.find (e) == set_.end ()) {
    set_.insert (e);
    queue_.push_back (e);
  }
}

void
fifo_scheduler::remove (void* action_entry_point,
			parameter_t parameter)
{
  entry e (action_entry_point, parameter);
  set_type::iterator pos = set_.find (e);
  if (pos != set_.end ()) {
    set_.erase (pos);

    if (queue_.front () == e) {
      queue_.pop_front ();
    }
    else {
      queue_.erase (std::find (queue_.begin (), queue_.end (), e));
    }
  }
}

void
fifo_scheduler::finish (bool output_status,
			value_t output_value)
{
  if (!queue_.empty ()) {
    /* Schedule. */
    entry e (queue_.front ());
    sys_schedule (e.action_entry_point, e.parameter, output_status, output_value);
  }
  else {
    /* Don't schedule. */
    sys_finish (output_status, output_value);
  }
}
