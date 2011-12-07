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
#include "action_macros.hpp"

template <class Alloc, template <typename> class Allocator>
class fifo_scheduler {
private:

  struct entry {
    const void* action_entry_point;
    aid_t parameter;

    entry (const void* aep,
	   aid_t p) :
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

  void
  add_ (const void* action_entry_point,
	 aid_t parameter)
  {
    entry e (action_entry_point, parameter);
    if (set_.find (e) == set_.end ()) {
      set_.insert (e);
      queue_.push_back (e);
    }
  }

  void
  remove_ (const void* action_entry_point,
	    aid_t parameter)
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
  finish_ (const void* buffer = 0)
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

public:

  fifo_scheduler (Alloc& a) :
    queue_ (typename queue_type::allocator_type (a)),
    set_ (3, typename set_type::hasher (), typename set_type::key_equal (), typename set_type::allocator_type (a))
  { }

  template <class LocalAction>
  void
  add (void (*action_entry_point) (void))
  {
    STATIC_ASSERT (is_local_action<LocalAction>::value);
    STATIC_ASSERT (LocalAction::parameter_mode == NO_PARAMETER);
    add_ (reinterpret_cast<const void*> (action_entry_point), 0);
  }

  template <class LocalAction>
  void
  add (void (*action_entry_point) (typename LocalAction::parameter_type),
       typename LocalAction::parameter_type parameter)
  {
    STATIC_ASSERT (is_local_action<LocalAction>::value);
    STATIC_ASSERT (LocalAction::parameter_mode == PARAMETER || LocalAction::parameter_mode == AUTO_PARAMETER);
    add_ (reinterpret_cast<const void*> (action_entry_point), reinterpret_cast<aid_t> (parameter));
  }

  template <class LocalAction>
  class remover {
  private:
    fifo_scheduler<Alloc, Allocator>& sched_;
    const void* action_entry_point_;

  public:
    remover (fifo_scheduler<Alloc, Allocator>& sched,
	     void (*ptr) (void)) :
      sched_ (sched),
      action_entry_point_ (reinterpret_cast<const void*> (ptr))
    { }

    void
    operator() ()
    {
      STATIC_ASSERT (is_local_action<LocalAction>::value);
      STATIC_ASSERT (LocalAction::parameter_mode == NO_PARAMETER);
      sched_.remove_ (action_entry_point_, 0);
    }

    void
    operator() (typename LocalAction::parameter_type parameter)
    {
      STATIC_ASSERT (is_local_action<LocalAction>::value);
      STATIC_ASSERT (LocalAction::parameter_mode == PARAMETER || LocalAction::parameter_mode == AUTO_PARAMETER);
      sched_.remove_ (action_entry_point_, reinterpret_cast<aid_t> (parameter));
    }
  };

  template <class LocalAction>
  remover<LocalAction>
  remove (void (*ptr) (void)) {
    return remover<LocalAction> (*this, ptr);
  }

  template <class LocalAction>
  remover<LocalAction>
  remove (void (*ptr) (typename LocalAction::parameter_type)) {
    return remover<LocalAction> (*this, ptr);
  }

  class finisher {
  private:
    fifo_scheduler<Alloc, Allocator>& sched_;

  public:
    finisher (fifo_scheduler<Alloc, Allocator>& sched) :
      sched_ (sched)
    { }

    void
    operator() ()
    {
      sched_.finish_ ();
    }

    void
    operator() (const void* buffer)
    {
      sched_.finish_ (buffer);
    }
  };

  finisher
  finish () {
    return finisher (*this);
  }
};

#endif /* __fifo_scheduler_hpp__ */
