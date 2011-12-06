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
#include "kassert.hpp"

template <class Alloc, template <typename> class Allocator>
class fifo_scheduler {
private:

  struct entry {
    size_t action_entry_point;
    void* parameter;

    entry (size_t aep,
	   void* p) :
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

  fifo_scheduler (Alloc& a) :
    queue_ (typename queue_type::allocator_type (a)),
    set_ (3, typename set_type::hasher (), typename set_type::key_equal (), typename set_type::allocator_type (a))
  { }

  template <class LocalAction>
  void
  add (typename LocalAction::parameter_type parameter)
  {
    entry e (LocalAction::action_entry_point, reinterpret_cast<void*> (parameter));
    if (set_.find (e) == set_.end ()) {
      set_.insert (e);
      queue_.push_back (e);
    }
  }
  
  void
  remove (size_t action_entry_point,
	  void* parameter)
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
  finish (void* buffer)
  {
    if (!queue_.empty ()) {
      /* Schedule. */
      const entry& e = queue_.front ();
      sys_finish (e.action_entry_point, e.parameter, buffer);
    }
    else {
      /* Don't schedule. */
      sys_finish (0, 0, buffer);
    }
  }

};

#endif /* __fifo_scheduler_hpp__ */
