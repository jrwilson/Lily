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

#include <deque>
#include <unordered_set>
#include "syscall.hpp"
#include "action_traits.hpp"

template <template <typename> class Allocator>
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
    const entry e (action_entry_point, parameter);
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
  finish_ (bool status,
	   bid_t bid,
	   const void* buffer)
  {
    if (!queue_.empty ()) {
      /* Schedule. */
      const entry& e = queue_.front ();
      system::finish (e.action_entry_point, e.parameter, status, bid, buffer);
    }
    else {
      /* Don't schedule. */
      system::finish (0, 0, status, bid, buffer);
    }
  }

public:
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
    add_ (reinterpret_cast<const void*> (action_entry_point), aid_cast (parameter));
  }

  template <class LocalAction>
  void
  remove (void (*action_entry_point) (void))
  {
    STATIC_ASSERT (is_local_action<LocalAction>::value);
    STATIC_ASSERT (LocalAction::parameter_mode == NO_PARAMETER);
    remove_ (reinterpret_cast<const void*> (action_entry_point), 0);
  }
  
  template <class LocalAction>
  void
  remove (void (*action_entry_point) (typename LocalAction::parameter_type),
	  typename LocalAction::parameter_type parameter)
  {
    STATIC_ASSERT (is_local_action<LocalAction>::value);
    STATIC_ASSERT (LocalAction::parameter_mode == PARAMETER || LocalAction::parameter_mode == AUTO_PARAMETER);
    remove_ (reinterpret_cast<const void*> (action_entry_point), aid_cast (parameter));
  }

  template <class Action>
  void
  finish ()
  {
    STATIC_ASSERT (is_action<Action>::value);
    finish_ (false, -1, 0);
  }

  template <class OutputAction>
  void
  finish (bool status)
  {
    STATIC_ASSERT (is_output_action<OutputAction>::value && OutputAction::buffer_value_mode == NO_BUFFER_VALUE && OutputAction::copy_value_mode == NO_COPY_VALUE);
    finish_ (status, -1, 0);
  }

  template <class OutputAction>
  void
  finish (bid_t bid)
  {
    STATIC_ASSERT (is_output_action<OutputAction>::value && OutputAction::buffer_value_mode == BUFFER_VALUE && OutputAction::copy_value_mode == NO_COPY_VALUE);
    finish_ (true, bid, 0);
  }

  template <class OutputAction>
  void
  finish (const typename OutputAction::copy_value_type* buffer)
  {
    STATIC_ASSERT (is_output_action<OutputAction>::value && OutputAction::buffer_value_mode == NO_BUFFER_VALUE && OutputAction::copy_value_mode == COPY_VALUE);
    finish_ (true, -1, buffer);
  }

  template <class OutputAction>
  void
  finish (bid_t bid,
	  const typename OutputAction::copy_value_type* buffer)
  {
    STATIC_ASSERT (is_output_action<OutputAction>::value && OutputAction::buffer_value_mode == BUFFER_VALUE && OutputAction::copy_value_mode == COPY_VALUE);
    finish_ (true, bid, buffer);
  }

};

#endif /* __fifo_scheduler_hpp__ */
