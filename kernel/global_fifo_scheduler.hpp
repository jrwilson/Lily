#ifndef __global_fifo_scheduler_hpp__
#define __global_fifo_scheduler_hpp__

/*
  File
  ----
  global_fifo_scheduler.hpp
  
  Description
  -----------
  A round-robin scheduler.

  Below, you will see comments like +AAA and -AAA.
  These are reference counting and locking markup where -AAA reverses the effect of +AAA.

  EEE - Lock the automaton containing the local action.
  FFF - Lock the automata containing input actions.

  Authors:
  Justin R. Wilson
*/

#include "action.hpp"
#include "vm.hpp"
#include "automaton.hpp"
#include "string.hpp"
#include "linear_set.hpp"

class global_fifo_scheduler {
private:

  // Align the stack when executing.
  static const size_t STACK_ALIGN = 16;

  typedef linear_set<caction, caction_hash> automaton_context;

  // Map an automaton to its scheduling context.
  typedef unordered_map<shared_ptr<automaton>, automaton_context*> context_map_type;
  static context_map_type context_map_;

  // Queue of automaton with actions to execute.
  typedef linear_set<automaton_context*> queue_type;
  static queue_type ready_queue_;

  // The action that is currently executing.
  static caction action_;

  // List of input actions to be used when executing bound output actions.
  typedef vector<shared_ptr<binding> > input_action_list_type;
  static input_action_list_type input_action_list_;

  // Iterator that marks our progress when executing input actions.
  static input_action_list_type::const_iterator input_action_pos_;

  // Buffers produced by an output action that will be copied to the input action.
  static shared_ptr<buffer> output_buffer_a_;
  static shared_ptr<buffer> output_buffer_b_;

  struct sort_bindings_by_input {
    bool
    operator () (const shared_ptr<binding>& x,
		 const shared_ptr<binding>& y) const
    {
      return x->input_action.automaton->aid () < y->input_action.automaton->aid ();
    }
  };

  static inline void
  proceed_to_input (void)
  {
    // Do not use temporary shared_ptr<binding> because it will not be destroyed if execute is called.
    while (input_action_pos_ != input_action_list_.end ()) {
      if ((*input_action_pos_)->enabled ()) {
	action_ = (*input_action_pos_)->input_action;
	// This does not return.
	action_.automaton->execute (*action_.action, action_.parameter, output_buffer_a_, output_buffer_b_);
      }
      else {
	++input_action_pos_;
      }
    }
  }

  static inline void
  finish_output (void)
  {
    for (input_action_list_type::const_iterator pos = input_action_list_.begin ();
	 pos != input_action_list_.end ();
	 ++pos) {
      // -FFF
      (*pos)->input_action.automaton->unlock_execution ();
    }
    input_action_list_.clear ();
  }

public:
  static void
  initialize ()
  {
  }

  static inline void
  add_automaton (const shared_ptr<automaton>& a)
  {
    // Allocate a new context and insert it into the map.
    // Inserting should succeed.
    automaton_context* c = new automaton_context ();
    pair<context_map_type::iterator, bool> r = context_map_.insert (make_pair (a, c));
    kassert (r.second);
  }

  static inline void
  remove_automaton (const shared_ptr<automaton>& a)
  {
    context_map_type::iterator pos = context_map_.find (a);
    kassert (pos != context_map_.end ());
    automaton_context* c = pos->second;
    kassert (c != 0);
    context_map_.erase (pos);
    ready_queue_.erase (c);
    delete c;
  }

  static inline const shared_ptr<automaton>&
  current_automaton ()
  {
    kassert (action_.automaton.get () != 0);
    return action_.automaton;
  }

  static inline void
  schedule (const caction& ad)
  {
    context_map_type::iterator pos = context_map_.find (ad.automaton);
    kassert (pos != context_map_.end ());
    
    automaton_context* c = pos->second;
    !c->push_back (ad);
    
    ready_queue_.push_back (c);
  }

  static inline void
  schedule_irq (const caction& ad)
  {
    context_map_type::iterator pos = context_map_.find (ad.automaton);
    kassert (pos != context_map_.end ());
    
    automaton_context* c = pos->second;
    c->push_front (ad);
    
    ready_queue_.push_front (c);
  }
  
  static inline void
  finish (int output_fired,
	  bd_t bda,
	  bd_t bdb)
  {
    if (action_.automaton.get () != 0) {
      // We were executing an action of this automaton.

      switch (action_.action->type) {
      case INPUT:
	// We were executing an input.  Move to the next input.
	++input_action_pos_;
	proceed_to_input ();
	// -EEE
	input_action_list_.front ()->output_action.automaton->unlock_execution ();
	finish_output ();
	break;
      case OUTPUT:
	// We were executing an output ...
	if (output_fired) {
	  // ... and the output output did something.
	  output_buffer_a_ = action_.automaton->lookup_buffer (bda);
	  // Synchronize the buffers.
	  if (output_buffer_a_.get () != 0) {
	    output_buffer_a_->sync (0, output_buffer_a_->size ());
	  }
	  output_buffer_b_ = action_.automaton->lookup_buffer (bdb);
	  if (output_buffer_b_.get () != 0) {
	    output_buffer_b_->sync (0, output_buffer_b_->size ());
	  }
	  // Proceed to execute the inputs.
	  input_action_pos_ = input_action_list_.begin ();
	  // This does not return if there are inputs.
	  proceed_to_input ();
	}
	// No input actions to execute.
	// -EEE
	action_.automaton->unlock_execution ();
	finish_output ();
	break;
      case INTERNAL:
	// -EEE
	action_.automaton->unlock_execution ();
	break;
      }
    }

    // We are done with the current action.
    action_.automaton = shared_ptr<automaton> ();

    for (;;) {

      irq_handler::process_interrupts ();

      if (!ready_queue_.empty ()) {
	// Get the automaton context and remove it from the ready queue.
	automaton_context* c = ready_queue_.front ();
	ready_queue_.pop_front ();

	// Load the action.
	action_ = c->front ();
	c->pop_front ();

	// The automaton exists.  Continue loading and execute.
	switch (action_.action->type) {
	case INPUT:
	  // Error.  Not a local action.
	  kpanic ("Non-local action on execution queue");
	  break;
	case OUTPUT:
	  {
	    kassert (input_action_list_.empty ());
	    // Copy the bindings.
	    action_.automaton->copy_bound_inputs (action_, back_inserter (input_action_list_));
	    
	    // Sort the bindings by input automaton.
	    sort (input_action_list_.begin (), input_action_list_.end (), sort_bindings_by_input ());
	    
	    // We lock the automata in order.  This is called Havender's Principle.
	    bool output_locked = false;
	    for (input_action_list_type::const_iterator pos = input_action_list_.begin ();
		 pos != input_action_list_.end ();
		 ++pos) {
	      shared_ptr<automaton> input_automaton = (*pos)->input_action.automaton;
	      if (!output_locked && action_.automaton->aid () < input_automaton->aid ()) {
		// +EEE
		action_.automaton->lock_execution ();
		output_locked = true;
	      }
	      // +FFF
	      input_automaton->lock_execution ();
	    }
	    if (!output_locked) {
	      // +EEE
	      action_.automaton->lock_execution ();
	      output_locked = true;
	    }
	    
	    input_action_pos_ = input_action_list_.begin ();
	  }
	  break;
	case INTERNAL:
	  // +EEE
	  action_.automaton->lock_execution ();
	  break;
	}
	
	if (!c->empty ()) {
	  // Automaton has more actions, return to ready queue.
	  ready_queue_.push_back (c);
	}

	// This call does not return.
	action_.automaton->execute (*action_.action, action_.parameter, output_buffer_a_, output_buffer_b_);
      }

      // Out of actions.
      action_.automaton = shared_ptr<automaton> ();
      irq_handler::wait_for_interrupt ();
    }
  }

};

#endif /* __global_fifo_scheduler_hpp__ */
