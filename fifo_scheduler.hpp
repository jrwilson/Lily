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

  typedef std::deque<entry> queue_type;
  queue_type queue_;
  typedef std::unordered_set<entry> set_type;
  set_type set_;

public:

  void
  add (void* action_entry_point,
       parameter_t parameter);

  void
  remove (void* action_entry_point,
	  parameter_t parameter);

  void
  finish (bool output_status,
	  value_t output_value);
};

#endif /* __fifo_scheduler_hpp__ */
