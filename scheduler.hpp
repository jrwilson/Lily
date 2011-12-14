#ifndef __scheduler_hpp__
#define __scheduler_hpp__

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

#include <deque>
#include "binding_manager.hpp"
#include "system_automaton.hpp"

// Size of the stack to use when switching between automata.
// Note that this is probably not the number of bytes as each element in the array may occupy more than one byte.
static const size_t SWITCH_STACK_SIZE = 256;

// Align the stack when switching and executing.
static const size_t STACK_ALIGN = 16;

class scheduler {
private:
  enum status_t {
    SCHEDULED,
    NOT_SCHEDULED
  };

  struct action_parameter {
    const void* action_entry_point;
    aid_t parameter;
    
    action_parameter (const void* aep,
		      aid_t p) :
      action_entry_point (aep),
      parameter (p)
    { }
    
    bool
    operator== (const action_parameter& other) const
    {
      return action_entry_point == other.action_entry_point && parameter == other.parameter;
    }
  };

  class automaton_context {
  private:
    typedef std::deque<action_parameter, system_allocator<action_parameter> > queue_type;
    queue_type queue_;
    typedef std::unordered_set<action_parameter, std::hash<action_parameter>, std::equal_to<action_parameter>, system_allocator<action_parameter> > set_type;
    set_type set_;
    
  public:
    status_t status;
    ::automaton* automaton;
    
    automaton_context (::automaton* a) :
      status (NOT_SCHEDULED),
      automaton (a)
    { }

    void
    push (const action_parameter& ap)
    {
      if (set_.find (ap) == set_.end ()) {
  	set_.insert (ap);
  	queue_.push_back (ap);
      }	
    }

    void
    pop ()
    {
      const size_t count = set_.erase (queue_.front ());
      kassert (count == 1);
      queue_.pop_front ();
    }

    const action_parameter&
    front () const
    {
      return queue_.front ();
    }
  };
  
  class execution_context {
  private:
    uint32_t switch_stack_[SWITCH_STACK_SIZE];
    uint8_t value_buffer_[VALUE_BUFFER_SIZE];

    automaton* automaton_;
    
    action_type_t type_;
    const void* action_entry_point_;
    parameter_mode_t parameter_mode_;
    size_t value_size_;

    aid_t parameter_;

    const binding_manager::input_action_set_type* input_actions_;
    binding_manager::input_action_set_type::const_iterator input_action_pos_;

    void
    exec (uint32_t end_of_stack = -1) const
    {
      uint32_t* stack_begin;
      uint32_t* stack_end;
      uint32_t* new_stack_begin;
      size_t stack_size;
      size_t idx;
      uint32_t stack_segment = automaton_->stack_segment ();
      uint32_t* stack_pointer = const_cast<uint32_t*> (static_cast<const uint32_t*> (automaton_->stack_pointer ()));
      uint32_t code_segment = automaton_->code_segment ();

      // Move the stack into an area mapped in all address spaces so that switching page directories doesn't cause a triple fault.
  
      // Determine the beginning of the stack.
      asm ("mov %%esp, %0\n" : "=g"(stack_begin));

      // Determine the end of the stack.
      stack_end = &end_of_stack + 1;

      // Compute the size of the stack.
      stack_size = stack_end - stack_begin;

      new_stack_begin = static_cast<uint32_t*> (const_cast<void*> (align_down (switch_stack_ + SWITCH_STACK_SIZE - stack_size, STACK_ALIGN)));

      // Check that the stack will work.
      kassert (new_stack_begin >= switch_stack_);

      // Copy.
      for (idx = 0; idx < stack_size; ++idx) {
	new_stack_begin[idx] = stack_begin[idx];
      }
  
      /* Update the base and stack pointers. */
      asm ("add %0, %%esp\n"
	   "add %0, %%ebp\n" :: "r"((new_stack_begin - stack_begin) * sizeof (uint32_t)) : "%esp", "memory");

      vm::switch_to_directory (automaton_->page_directory_frame ());

      if (type_ == INPUT) {
	// Copy the value to the stack.
	stack_pointer = static_cast<uint32_t*> (const_cast<void*> (align_down (reinterpret_cast<uint8_t*> (stack_pointer) - value_size_, STACK_ALIGN)));
	memcpy (stack_pointer, value_buffer_, value_size_);
      }

      switch (parameter_mode_) {
      case NO_PARAMETER:
	break;
      case PARAMETER:
      case AUTO_PARAMETER:
	  // Copy the parameter to the stack.
	  *--stack_pointer = static_cast<uint32_t> (parameter_);
	  break;
      }

      // These instructions are obviously important but also have the side effect of backing the stack with frames if it doesn't exist.

      // Push a bogus instruction pointer so we can use the cdecl calling convention.
      *--stack_pointer = 0;

      // Push the flags.
      uint32_t eflags;
      asm ("pushf\n"
	   "pop %0\n" : "=g"(eflags) : :);
      *--stack_pointer = eflags;

      // Push the code segment.
      *--stack_pointer = code_segment;

      // Push the instruction pointer.
      *--stack_pointer = reinterpret_cast<uint32_t> (action_entry_point_);

      asm ("mov %%eax, %%ss\n"	// Load the new stack segment.
	   "mov %%ebx, %%esp\n"	// Load the new stack pointer.
	   "xor %%eax, %%eax\n"	// Clear the registers.
	   "xor %%ebx, %%ebx\n"
	   "xor %%ecx, %%ecx\n"
	   "xor %%edx, %%edx\n"
	   "xor %%edi, %%edi\n"
	   "xor %%esi, %%esi\n"
	   "xor %%ebp, %%ebp\n"
	   "iret\n" :: "a"(stack_segment), "b"(stack_pointer));
    }
    
  public:
    execution_context ()
    {
      clear ();
    }

    void
    clear ()
    {
      automaton_ = 0;
    }

    automaton*
    current_automaton () const
    {
      kassert (automaton_ != 0);
      return automaton_;
    }

    void
    load (automaton_context* c)
    {
      action_parameter ap = c->front ();
      c->pop ();
      automaton::const_action_iterator pos = c->automaton->action_find (ap.action_entry_point);
      if (pos != c->automaton->action_end ()) {
	automaton_ = c->automaton;

	type_ = pos->type;
	action_entry_point_ = pos->action_entry_point;
	parameter_mode_ = pos->parameter_mode;
	value_size_ = pos->value_size;

	parameter_ = ap.parameter;
      }
      else {
	clear ();
      }

      c->status = NOT_SCHEDULED;
    }

    void
    finish_action (bool output_status,
		   const void* buffer)
    {
      if (automaton_ != 0) {	
	switch (type_) {
	case INPUT:
	  /* Move to the next input. */
	  ++input_action_pos_;
	  if (input_action_pos_ != input_actions_->end ()) {
	    /* Load the execution context. */
	    automaton_ = input_action_pos_->automaton;
	    
	    action_entry_point_ = input_action_pos_->action_entry_point;
	    parameter_mode_ = input_action_pos_->parameter_mode;
	    
	    parameter_ = input_action_pos_->parameter;
	    /* Execute.  (This does not return). */
	    exec ();
	  }
	  break;
	case OUTPUT:
	  // Only proceed if the output executed.
	  // If the output should have produced data (value_size_ != 0), then check that the supplied buffer is valid.
	  // Finally, the the inputs bound to the output.  If an input exists, proceed with execution.
	  if (output_status) {
	    if (value_size_ == 0 || automaton_->verify_span (buffer, value_size_)) {
	      input_actions_ = binding_manager::get_bound_inputs (automaton_, action_entry_point_, parameter_mode_, parameter_);
	      if (input_actions_ != 0) {
		// Copy the value.
		memcpy (value_buffer_, buffer, value_size_);
		
		/* Load the execution context. */
		input_action_pos_ = input_actions_->begin ();
		automaton_ = input_action_pos_->automaton;
		
		type_ = INPUT;
		action_entry_point_ = input_action_pos_->action_entry_point;
		parameter_mode_ = input_action_pos_->parameter_mode;
		
		parameter_ = input_action_pos_->parameter;
		/* Execute.  (This does not return). */
		exec ();
	      }
	    }
	    else {
	      // The value produced by the output function is bad.
	      system_automaton::bad_value ();
	    }
	  }
	  break;
	case INTERNAL:
	  break;
	}
      }
    }

    void
    execute () const
    {
      if (automaton_ != 0) {
	switch (type_) {
	case OUTPUT:
	case INTERNAL:
	  exec ();
	  break;
	case INPUT:
	  break;
	}
      }
    }

  };

  /* TODO:  Need one per core. */
  execution_context exec_context_;
  typedef std::deque<automaton_context*, system_allocator<automaton_context*> > queue_type;
  queue_type ready_queue_;
  typedef std::unordered_map<automaton*, automaton_context*, std::hash<automaton*>, std::equal_to<automaton*>, system_allocator<std::pair<automaton* const, automaton_context*> > > context_map_type;
  context_map_type context_map_;

  void
  switch_to_next_action ()
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

public:
  void
  add_automaton (automaton* automaton)
  {
    // Allocate a new context and insert it into the map.
    // Inserting should succeed.
    automaton_context* c = new (system_alloc ()) automaton_context (automaton);
    std::pair<context_map_type::iterator, bool> r = context_map_.insert (std::make_pair (automaton, c));
    kassert (r.second);
  }

  automaton*
  current_automaton () const
  {
    return exec_context_.current_automaton ();
  }

  void
  schedule_action (automaton* automaton,
		   const void* action_entry_point,
		   aid_t parameter)
  {
    context_map_type::iterator pos = context_map_.find (automaton);
    kassert (pos != context_map_.end ());

    automaton_context* c = pos->second;

    c->push (action_parameter (action_entry_point, parameter));
    
    if (c->status == NOT_SCHEDULED) {
      c->status = SCHEDULED;
      ready_queue_.push_back (c);
    }
  }
  
  void
  finish (const void* action_entry_point,
	  aid_t parameter,
	  bool output_status,
	  const void* buffer)
  {
    if (action_entry_point != 0) {
      schedule_action (current_automaton (), action_entry_point, parameter);
    }
    exec_context_.finish_action (output_status, buffer);
    switch_to_next_action ();
  }

};

#endif /* __scheduler_hpp__ */
