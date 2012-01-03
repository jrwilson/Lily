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

  struct entry_hash {
    size_t
    operator() (const entry& e) const
    {
      return reinterpret_cast<size_t> (e.action_entry_point) ^ e.parameter;
    }
  };

  typedef std::deque<entry> queue_type;
  queue_type queue_;
  typedef std::unordered_set<entry, entry_hash> set_type;
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
  finish_ (bool status,
	   bid_t bid,
	   const void* buffer)
  {
    if (!queue_.empty ()) {
      /* Schedule. */
      const entry& e = queue_.front ();
      lilycall::finish (e.action_entry_point, e.parameter, status, bid, buffer);
    }
    else {
      /* Don't schedule. */
      lilycall::finish (0, -1, status, bid, buffer);
    }
  }

public:
  template <class LocalAction>
  void
  add (void (*action_entry_point) (void))
  {
    static_assert (is_local_action<LocalAction>::value && LocalAction::parameter_mode == NO_PARAMETER, "add expects an unparameterized local action");
    add_ (reinterpret_cast<const void*> (action_entry_point), 0);
  }

  template <class LocalAction>
  void
  add (void (*action_entry_point) (typename LocalAction::parameter_type),
       typename LocalAction::parameter_type parameter)
  {
    static_assert (is_local_action<LocalAction>::value && (LocalAction::parameter_mode == PARAMETER || LocalAction::parameter_mode == AUTO_PARAMETER), "add expects a parameterized local action");
    add_ (reinterpret_cast<const void*> (action_entry_point), aid_cast (parameter));
  }

  template <class LocalAction>
  void
  remove (void (*action_entry_point) (void))
  {
    static_assert (is_local_action<LocalAction>::value && LocalAction::parameter_mode == NO_PARAMETER, "remove expects an unparameterized local action");
    remove_ (reinterpret_cast<const void*> (action_entry_point), 0);
  }
  
  template <class LocalAction>
  void
  remove (void (*action_entry_point) (typename LocalAction::parameter_type),
	  typename LocalAction::parameter_type parameter)
  {
    static_assert (is_local_action<LocalAction>::value && (LocalAction::parameter_mode == PARAMETER || LocalAction::parameter_mode == AUTO_PARAMETER), "remove expects a parameterized local action");
    remove_ (reinterpret_cast<const void*> (action_entry_point), aid_cast (parameter));
  }

  template <class Action>
  void
  finish ()
  {
    static_assert (is_action<Action>::value, "Action is not an action");
    finish_ (false, -1, 0);
  }

  template <class OutputAction>
  void
  finish (bool status)
  {
    static_assert (is_output_action<OutputAction>::value && OutputAction::buffer_value_mode == NO_BUFFER_VALUE && OutputAction::copy_value_mode == NO_COPY_VALUE, "finish expects an output action without a buffer or copy value");
    finish_ (status, -1, 0);
  }

  template <class OutputAction>
  void
  finish (bid_t bid)
  {
    static_assert (is_output_action<OutputAction>::value && OutputAction::buffer_value_mode == BUFFER_VALUE && OutputAction::copy_value_mode == NO_COPY_VALUE, "finish expects output action with buffer value");
    finish_ (true, bid, 0);
  }

  template <class OutputAction>
  void
  finish (const typename OutputAction::copy_value_type* buffer)
  {
    static_assert (is_output_action<OutputAction>::value && OutputAction::buffer_value_mode == NO_BUFFER_VALUE && OutputAction::copy_value_mode == COPY_VALUE, "finish expects output action with copy value");
    finish_ (true, -1, buffer);
  }

  template <class OutputAction>
  void
  finish (bid_t bid,
	  const typename OutputAction::copy_value_type* buffer)
  {
    static_assert (is_output_action<OutputAction>::value && OutputAction::buffer_value_mode == BUFFER_VALUE && OutputAction::copy_value_mode == COPY_VALUE, "finish expects output action with buffer and copy values");
    finish_ (true, bid, buffer);
  }

};

#endif /* __fifo_scheduler_hpp__ */
