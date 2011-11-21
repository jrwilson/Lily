/*
  File
  ----
  scheduler.c
  
  Description
  -----------
  Scheduling functions.

  Authors
  -------
  Justin R. Wilson
*/

#include "scheduler.hpp"
#include "kassert.hpp"
#include "idt.hpp"
#include "system_automaton.hpp"
#include "binding_manager.hpp"

scheduler::queue_type* scheduler::ready_queue_ = 0;
scheduler::execution_context scheduler::exec_context;

void
scheduler::initialize (automaton_interface* automaton)
{
  kassert (automaton != 0);

  exec_context.action_type = NO_ACTION;
  exec_context.current_automaton = automaton;

  ready_queue_ = new queue_type;
}

void
scheduler::set_switch_stack (logical_address switch_stack,
			     size_t switch_stack_size)
{
  kassert (switch_stack != logical_address ());
  kassert (switch_stack >= KERNEL_VIRTUAL_BASE);
  kassert (switch_stack_size > 0);

  exec_context.switch_stack = switch_stack;
  exec_context.switch_stack_size = switch_stack_size;
}

scheduler::automaton_context*
scheduler::allocate_context (automaton_interface* automaton)
{
  kassert (automaton != 0);
  return new automaton_context (automaton);
}

automaton_interface*
scheduler::get_current_automaton (void)
{
  kassert (exec_context.current_automaton != 0);
  return exec_context.current_automaton;
}

void
scheduler::schedule_action (automaton_interface* automaton,
			    void* action_entry_point,
			    parameter_t parameter)
{
  kassert (ready_queue_ != 0);
  kassert (automaton != 0);
  automaton_context* ptr = automaton->get_scheduler_context ();
  kassert (ptr != 0);

  ptr->action_entry_point = action_entry_point;
  ptr->parameter = parameter;
  
  if (ptr->status == NOT_SCHEDULED) {
    ptr->status = SCHEDULED;
    ready_queue_->push_back (ptr);
  }
}

void
scheduler::switch_to_next_action ()
{
  if (!ready_queue_->empty ()) {
    /* Load the execution context. */
    automaton_context* ptr = ready_queue_->front ();
    ready_queue_->pop_front ();
    ptr->status = NOT_SCHEDULED;
    exec_context.current_automaton = ptr->automaton_;
    exec_context.current_action_entry_point = ptr->action_entry_point;
    exec_context.current_parameter = ptr->parameter;

    /* Get the type to make sure its a local action. */
    exec_context.action_type = exec_context.current_automaton->get_action_type (exec_context.current_action_entry_point);
    switch (exec_context.action_type) {
    case OUTPUT:
    case INTERNAL:
      /* Execute.  (This does not return). */
      exec_context.current_automaton->execute (exec_context.switch_stack, exec_context.switch_stack_size, exec_context.current_action_entry_point, exec_context.current_parameter, 0);
      break;
    case INPUT:
    case NO_ACTION:
      /* TODO:  Kill the automaton for scheduling a bad action. */
      kassert (0);
      /* Recur to get the next. */
      switch_to_next_action ();
      break;
    }
  }
  else {
    /* Out of actions.  Halt. */
    exec_context.action_type = NO_ACTION;
    enable_interrupts ();
    asm volatile ("hlt");
  }
}

void
scheduler::finish_action (int output_status,
			  value_t output_value)
{
  switch (exec_context.action_type) {
  case INPUT:
    /* Move to the next input. */
    ++exec_context.input_action_pos;
    if (exec_context.input_action_pos != exec_context.input_actions->end ()) {
      /* Load the execution context. */
      exec_context.current_automaton = exec_context.input_action_pos->automaton;
      exec_context.current_action_entry_point = exec_context.input_action_pos->action_entry_point;
      exec_context.current_parameter = exec_context.input_action_pos->parameter;
      /* Execute.  (This does not return). */
      exec_context.current_automaton->execute (exec_context.switch_stack, exec_context.switch_stack_size, exec_context.current_action_entry_point, exec_context.current_parameter, exec_context.output_value);
    }
    break;
  case OUTPUT:
    if (output_status) {
      /* Get the inputs that are composed with the output. */
      exec_context.input_actions = binding_manager::get_bound_inputs (exec_context.current_automaton, exec_context.current_action_entry_point, exec_context.current_parameter);
      if (exec_context.input_actions != 0) {
	/* The output executed and there are inputs. */
	exec_context.input_action_pos = exec_context.input_actions->begin ();
	exec_context.output_value = output_value;
	/* Load the execution context. */
	exec_context.action_type = INPUT;
	exec_context.current_automaton = exec_context.input_action_pos->automaton;
	exec_context.current_action_entry_point = exec_context.input_action_pos->action_entry_point;
	exec_context.current_parameter = exec_context.input_action_pos->parameter;
	/* Execute.  (This does not return). */
	exec_context.current_automaton->execute (exec_context.switch_stack, exec_context.switch_stack_size, exec_context.current_action_entry_point, exec_context.current_parameter, exec_context.output_value);
      }
    }
    break;
  case INTERNAL:
  case NO_ACTION:
    break;
  }

  switch_to_next_action ();
}
