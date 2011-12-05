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

// Align the stack when switching.
static const size_t STACK_ALIGN = 16;

scheduler::execution_context::execution_context (binding_manager& bm) :
  binding_manager_ (bm)
{
  clear ();
}

void
scheduler::execution_context::clear ()
{
  automaton_ = 0;
  action_ = automaton::action ();
  parameter_ = 0;
}

automaton*
scheduler::execution_context::current_automaton () const
{
  kassert (automaton_ != 0);
  return automaton_;
}

void
scheduler::execution_context::load (automaton_context* c)
{
  automaton_ = c->automaton;
  action_ = automaton_->get_action (c->action);
  parameter_ = c->parameter;
  c->status = NOT_SCHEDULED;
}

void
scheduler::execution_context::exec (int end_of_stack) const
{
  logical_address stack_begin;
  logical_address stack_end;
  logical_address new_stack_begin;
  size_t stack_size;
  size_t idx;
  uint32_t stack_segment = automaton_->stack_segment ();
  logical_address stack_pointer = automaton_->stack_pointer ();
  uint32_t code_segment = automaton_->code_segment ();

  /* Move the stack into an area mapped in all address spaces so that switching page directories doesn't cause a triple fault. */
  
  /* Determine the beginning of the stack. */
  asm volatile ("mov %%esp, %0\n" : "=m"(stack_begin));
  stack_begin += sizeof (uint32_t);

  /* Determine the end of the stack. */
  stack_end = logical_address (&end_of_stack);
  stack_end += sizeof (end_of_stack);
  /* Compute the beginning of the new stack. */
  stack_size = stack_end - stack_begin;

  /* Use a bigger switch stack. */
  kassert (stack_size < sizeof (switch_stack_));
  new_stack_begin = (logical_address (switch_stack_) + (sizeof (switch_stack_) - stack_size)) >> STACK_ALIGN;
  /* Copy. */
  for (idx = 0; idx < stack_size; ++idx) {
    new_stack_begin[idx] = stack_begin[idx];
  }
  
  /* Update the base and stack pointers. */
  asm volatile ("add %0, %%esp\n"
		"add %0, %%ebp\n" :: "r"(new_stack_begin - stack_begin) : "%esp", "memory");

  /* Switch page directories. */
  vm_manager::switch_to_directory (automaton_->page_directory ());

  switch (action_.type) {
  case automaton::INPUT:
    // Adjust the stack pointer.
    stack_pointer -= action_.message_size;
    stack_pointer >>= STACK_ALIGN;
    // Copy the value to the stack.
    memcpy (stack_pointer.value (), message_buffer_, action_.message_size);
    asm volatile ("mov %0, %%eax\n"	// Load the new stack segment.
		  "mov %%ax, %%ss\n"
		  "mov %1, %%eax\n"	// Load the new stack pointer.
		  "mov %%eax, %%esp\n"
		  "mov %1, %%eax\n"	// Push the pointer to the value.
		  "push %%eax\n"	
		  "pushl %4\n"		// Push the parameter.
		  "sub $0x4, %%esp\n"	// Adjust the stack pointer for the cdecl calling convention.
		  "pushf\n"		// Push flags (EFLAGS).
		  "pushl %2\n"		// Push the code segment (CS).
		  "pushl %3\n"		// Push the instruction pointer (EIP).
		  "iret\n" :: "m"(stack_segment), "m"(stack_pointer), "m"(code_segment), "m"(action_.input), "m"(parameter_) : "%eax");
    break;
  case automaton::OUTPUT:
  case automaton::INTERNAL:
    asm volatile ("mov %0, %%eax\n"	// Load the new stack segment.
		  "mov %%ax, %%ss\n"
		  "mov %1, %%eax\n"	// Load the new stack pointer.
		  "mov %%eax, %%esp\n"
		  "pushl %4\n"		// Push the parameter.
		  "sub $0x4, %%esp\n"	// Adjust the stack pointer for the cdecl calling convention.
		  "pushf\n"		// Push flags (EFLAGS).
		  "pushl %2\n"		// Push the code segment (CS).
		  "pushl %3\n"		// Push the instruction pointer (EIP).
		  "iret\n" :: "m"(stack_segment), "m"(stack_pointer), "m"(code_segment), "m"(action_.output), "m"(parameter_) : "%eax");
    break;
  case automaton::NO_ACTION:
    // Error.
    kassert (0);
    break;
  }
}

void
scheduler::execution_context::finish_action (bool output_status,
					     void* buffer)
{
  switch (action_.type) {
  case automaton::INPUT:
    /* Move to the next input. */
    ++input_action_pos_;
    if (input_action_pos_ != input_actions_->end ()) {
      /* Load the execution context. */
      automaton_ = input_action_pos_->automaton;
      action_.input = input_action_pos_->input;
      parameter_ = input_action_pos_->parameter;
      /* Execute.  (This does not return). */
      exec ();
    }
    break;
  case automaton::OUTPUT:
    if (output_status) {
      input_actions_ = binding_manager_.get_bound_inputs (automaton_, action_.output, parameter_);
      if (input_actions_ != 0) {
	/* The output executed and there are inputs. */
	input_action_pos_ = input_actions_->begin ();
	/* Load the execution context. */
	automaton_ = input_action_pos_->automaton;
	action_.input = input_action_pos_->input;
	parameter_ = input_action_pos_->parameter;
	// We know its an input.
	action_.type = automaton::INPUT;
	// Copy the value.
	memcpy (message_buffer_, buffer, action_.message_size);
	/* Execute.  (This does not return). */
	exec ();
      }
    }
    break;
  case automaton::INTERNAL:
  case automaton::NO_ACTION:
    break;
  }
}

void
scheduler::execution_context::execute () const
{
  switch (action_.type) {
  case automaton::OUTPUT:
  case automaton::INTERNAL:
    exec ();
    break;
  case automaton::NO_ACTION:
  case automaton::INPUT:
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

    exec_context_.execute ();
    
    /* Since execute () returned, kill the automaton for scheduling a bad action. */
    system_automaton::bad_schedule ();
    /* Recur to get the next. */
    switch_to_next_action ();
  }
  else {
    /* Out of actions.  Halt. */
    exec_context_.clear ();
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
  return exec_context_.current_automaton ();
}

void
scheduler::schedule_action (automaton* automaton,
			    local_func action_entry_point,
			    void* parameter)
{
  context_map_type::iterator pos = context_map_.find (automaton);
  kassert (pos != context_map_.end ());

  automaton_context* c = pos->second;

  c->action = action_entry_point;
  c->parameter = parameter;
    
  if (c->status == NOT_SCHEDULED) {
    c->status = SCHEDULED;
    ready_queue_.push_back (c);
  }
}
  
void
scheduler::finish_action (bool schedule_status,
			  local_func action_entry_point,
			  void* parameter,
			  bool output_status,
			  void* buffer)
{
  if (schedule_status) {
    schedule_action (current_automaton (), action_entry_point, parameter);
  }
  exec_context_.finish_action (output_status, buffer);
  switch_to_next_action ();
}
