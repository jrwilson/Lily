/*
  File
  ----
  scheduler.hpp
  
  Description
  -----------
  Declarations for scheduling.

  Authors:
  Justin R. Wilson
*/

#include "scheduler.hpp"
#include "interrupts.hpp"
#include "system_automaton.hpp"

scheduler::execution_context::execution_context (binding_manager& bm) :
  binding_manager_ (bm)
{
  clear ();
}

void
scheduler::execution_context::clear ()
{
  current_action_ = action ();
  action_type_ = NO_ACTION;
  output_value_ = 0;
}

::automaton*
scheduler::execution_context::automaton () const
{
  kassert (current_action_.automaton != 0);
  return current_action_.automaton;
}

action_type_t
scheduler::execution_context::action_type () const
{
  return action_type_;
}

void
scheduler::execution_context::load (automaton_context* c)
{
  current_action_ = c->action;
  action_type_ = current_action_.automaton->get_action_type (current_action_.action_entry_point);
  output_value_ = 0;
      
  c->status = NOT_SCHEDULED;
}

void
scheduler::execution_context::execute () const
{
  current_action_.automaton->execute (logical_address (switch_stack_), sizeof (switch_stack_), current_action_.action_entry_point, current_action_.parameter, output_value_);
}

void
scheduler::execution_context::finish_action (bool output_status,
					     value_t output_value)
{
  switch (action_type_) {
  case INPUT:
    /* Move to the next input. */
    ++input_action_pos_;
    if (input_action_pos_ != input_actions_->end ()) {
      /* Load the execution context. */
      current_action_ = *input_action_pos_;
      /* Execute.  (This does not return). */
      execute ();
    }
    break;
  case OUTPUT:
    if (output_status) {
      input_actions_ = binding_manager_.get_bound_inputs (current_action_.automaton, current_action_.action_entry_point, current_action_.parameter);
      if (input_actions_ != 0) {
	/* The output executed and there are inputs. */
	input_action_pos_ = input_actions_->begin ();
	output_value_ = output_value;
	/* Load the execution context. */
	current_action_ = *input_action_pos_;
	// We know its an input.
	action_type_ = INPUT;
	/* Execute.  (This does not return). */
	execute ();
      }
    }
    break;
  case INTERNAL:
  case NO_ACTION:
    break;
  }
}

void
scheduler::switch_to_next_action ()
{
  if (!ready_queue_.empty ()) {
    /* Load the execution context. */
    exec_context_.load (ready_queue_.front ());
    ready_queue_.pop_front ();

    switch (exec_context_.action_type ()) {
    case OUTPUT:
    case INTERNAL:
      /* Execute.  (This does not return). */
      exec_context_.execute ();
      break;
    case INPUT:
    case NO_ACTION:
      /* Kill the automaton for scheduling a bad action. */
      system_automaton::bad_schedule ();
      /* Recur to get the next. */
      switch_to_next_action ();
      break;
    }
  }
  else {
    /* Out of actions.  Halt. */
    exec_context_.clear ();
    interrupts::enable ();
    asm volatile ("hlt");
  }
}

scheduler::scheduler (list_alloc& a,
		      binding_manager& bm) :
  alloc_ (a),
  exec_context_ (bm),
  ready_queue_ (queue_type::allocator_type (a)),
  context_map_ (3, context_map_type::hasher (), context_map_type::key_equal (), context_map_type::allocator_type (a))
{ }

void
scheduler::add_automaton (automaton* automaton)
{
  // Allocate a new context and insert it into the map.
  // Inserting should succeed.
  automaton_context* c = new (alloc_) automaton_context (automaton);
  std::pair<context_map_type::iterator, bool> r = context_map_.insert (std::make_pair (automaton, c));
  kassert (r.second);
}

automaton*
scheduler::current_automaton () const
{
  return exec_context_.automaton ();
}

void
scheduler::schedule_action (automaton* automaton,
			    void* action_entry_point,
			    parameter_t parameter)
{
  context_map_type::iterator pos = context_map_.find (automaton);
  kassert (pos != context_map_.end ());

  automaton_context* c = pos->second;

  c->action.action_entry_point = action_entry_point;
  c->action.parameter = parameter;
    
  if (c->status == NOT_SCHEDULED) {
    c->status = SCHEDULED;
    ready_queue_.push_back (c);
  }
}
  
void
scheduler::finish_action (bool output_status,
			  value_t output_value)
{
  exec_context_.finish_action (output_status, output_value);
  switch_to_next_action ();
}

