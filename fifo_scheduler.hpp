#ifndef __fifo_scheduler_hpp__
#define __fifo_scheduler_hpp__

/*
  File
  ----
  fifo_scheduler.hpp
  
  Description
  -----------
  A fifo scheduler for automata.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"
#include <deque>
#include <unordered_set>
#include "syscall.hpp"

template <class AllocatorTag, template <class> class Allocator>
class fifo_scheduler {
private:

  struct entry {
    void* action_entry_point;
    parameter_t parameter;

    entry (void* aep,
	   parameter_t p) :
      action_entry_point (aep),
      parameter (p)
    { }

    bool
    operator== (const entry& other) const
    {
      return action_entry_point == other.action_entry_point && parameter == other.parameter;
    }
  };

  typedef std::deque<entry, Allocator<entry> > queue_type;
  queue_type queue_;
  typedef std::unordered_set<entry, std::hash<entry>, std::equal_to<entry>, Allocator<entry> > set_type;
  set_type set_;

public:

  void
  add (void* action_entry_point,
       parameter_t parameter)
  {
    entry e (action_entry_point, parameter);
    if (set_.find (e) == set_.end ()) {
      set_.insert (e);
      queue_.push_back (e);
    }
  }
  
  void
  remove (void* action_entry_point,
	  parameter_t parameter)
  {
    entry e (action_entry_point, parameter);
    typename set_type::iterator pos = set_.find (e);
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
  finish (bool output_status,
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

};

#endif /* __fifo_scheduler_hpp__ */
