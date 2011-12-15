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

#include "action.hpp"
#include <deque>
#include "binding_manager.hpp"

class scheduler {
private:
  enum status_t {
    SCHEDULED,
    NOT_SCHEDULED
  };

  // Size of the stack to use when switching between automata.
  // Note that this is probably not the number of bytes as each element in the array may occupy more than one byte.
  static const size_t SWITCH_STACK_SIZE = 256;
  
  // Align the stack when switching and executing.
  static const size_t STACK_ALIGN = 16;

  class automaton_context {
  public:    
    automaton_context () :
      status_ (NOT_SCHEDULED)
    { }

    inline status_t
    status () const
    {
      return status_;
    }

    inline void
    status (status_t s)
    {
      status_ = s;
    }

    inline void
    push (const caction& ap)
    {
      if (set_.find (ap) == set_.end ()) {
  	set_.insert (ap);
  	queue_.push_back (ap);
      }	
    }

    inline void
    pop ()
    {
      const size_t count = set_.erase (queue_.front ());
      kassert (count == 1);
      queue_.pop_front ();
    }

    inline const caction&
    front () const
    {
      return queue_.front ();
    }

  private:
    status_t status_;
    typedef std::deque<caction, system_allocator<caction> > queue_type;
    queue_type queue_;
    typedef std::unordered_set<caction, std::hash<caction>, std::equal_to<caction>, system_allocator<caction> > set_type;
    set_type set_;
  };
  
  class execution_context {
  private:
    uint32_t switch_stack_[SWITCH_STACK_SIZE];
    uint8_t value_buffer_[VALUE_BUFFER_SIZE];

    caction action_;

    const binding_manager::input_action_set_type* input_actions_;
    binding_manager::input_action_set_type::const_iterator input_action_pos_;
    
  public:
    execution_context ()
    {
      clear ();
    }

    inline void
    clear ()
    {
      action_.automaton = 0;
    }

    inline automaton*
    current_automaton () const
    {
      kassert (action_.automaton != 0);
      return action_.automaton;
    }

    inline size_t
    current_value_size () const
    {
      return action_.value_size;
    }

    inline void
    load (const caction& ad)
    {
      action_ = ad;
    }

    inline void
    finish_action (bool output_status,
		   const void* buffer)
    {
      if (action_.automaton != 0) {	
	switch (action_.type) {
	case INPUT:
	  /* Move to the next input. */
	  ++input_action_pos_;
	  if (input_action_pos_ != input_actions_->end ()) {
	    /* Load the execution context. */
	    action_ = *input_action_pos_;
	    /* Execute.  (This does not return). */
	    execute ();
	  }
	  break;
	case OUTPUT:
	  // Only proceed if the output executed.
	  // If the output should have produced data (value_size_ != 0), then check that the supplied buffer is valid.
	  // Finally, the the inputs bound to the output.  If an input exists, proceed with execution.
	  if (output_status) {
	    input_actions_ = binding_manager::get_bound_inputs (action_);
	    if (input_actions_ != 0) {
	      // Copy the value.
	      memcpy (value_buffer_, buffer, action_.value_size);
	      
	      /* Load the execution context. */
	      input_action_pos_ = input_actions_->begin ();
	      action_ = *input_action_pos_;
	      /* Execute.  (This does not return). */
	      execute ();
	    }
	  }
	  break;
	case INTERNAL:
	  break;
	}
      }
    }

    inline void
    execute () const
    {
      // Move the stack into an area mapped in all address spaces so that switching page directories doesn't cause a triple fault.
  
      // Determine the beginning and end of the stack.
      uint32_t* stack_begin;
      uint32_t* stack_end;
      asm ("mov %%esp, %0\n"
	   "mov %%ebp, %1\n" : "=g"(stack_begin), "=g"(stack_end));
      // Follow the pointer to ensure that arguments are brought along in case of inlining.
      stack_end = reinterpret_cast<uint32_t*> (*stack_end);

      // Compute the size of the stack.
      size_t stack_size = stack_end - stack_begin;

      uint32_t* new_stack_begin = static_cast<uint32_t*> (const_cast<void*> (align_down (switch_stack_ + SWITCH_STACK_SIZE - stack_size, STACK_ALIGN)));

      // Check that the stack will work.
      kassert (new_stack_begin >= switch_stack_);

      // Copy.
      for (size_t idx = 0; idx < stack_size; ++idx) {
      	new_stack_begin[idx] = stack_begin[idx];
      }

      // Update the base and stack pointers.
      asm ("add %0, %%esp\n"
      	   "add %0, %%ebp\n" :: "r"((new_stack_begin - stack_begin) * sizeof (uint32_t)) : "%esp", "memory");

      // We can now switch page directories.
      vm::switch_to_directory (action_.automaton->page_directory_frame ());

      uint32_t* stack_pointer = const_cast<uint32_t*> (static_cast<const uint32_t*> (action_.automaton->stack_pointer ()));

      if (action_.type == INPUT) {
      	// Copy the value to the stack.
      	stack_pointer = static_cast<uint32_t*> (const_cast<void*> (align_down (reinterpret_cast<uint8_t*> (stack_pointer) - action_.value_size, STACK_ALIGN)));
      	memcpy (stack_pointer, value_buffer_, action_.value_size);
      }
      
      switch (action_.parameter_mode) {
      case NO_PARAMETER:
      	break;
      case PARAMETER:
      case AUTO_PARAMETER:
	// Copy the parameter to the stack.
	*--stack_pointer = static_cast<uint32_t> (action_.parameter);
	break;
      }

      // These instructions serve a dual purpose.
      // First, they set up the cdecl calling convention for actions.
      // Second, they force the stack to be created if it is not.

      // Push a bogus instruction pointer so we can use the cdecl calling convention.
      *--stack_pointer = 0;

      // Push the flags.
      uint32_t eflags;
      asm ("pushf\n"
      	   "pop %0\n" : "=g"(eflags) : :);
      *--stack_pointer = eflags;

      // Push the code segment.
      *--stack_pointer = action_.automaton->code_segment ();

      // Push the instruction pointer.
      *--stack_pointer = reinterpret_cast<uint32_t> (action_.action_entry_point);
      
      asm ("mov %%eax, %%ss\n"	// Load the new stack segment.
      	   "mov %%ebx, %%esp\n"	// Load the new stack pointer.
      	   "xor %%eax, %%eax\n"	// Clear the registers.
      	   "xor %%ebx, %%ebx\n"
      	   "xor %%ecx, %%ecx\n"
      	   "xor %%edx, %%edx\n"
      	   "xor %%edi, %%edi\n"
      	   "xor %%esi, %%esi\n"
      	   "xor %%ebp, %%ebp\n"
      	   "iret\n" :: "a"(action_.automaton->stack_segment ()), "b"(stack_pointer));
    }
  };

  /* TODO:  Need one per core. */
  execution_context exec_context_;
  typedef std::deque<automaton_context*, system_allocator<automaton_context*> > queue_type;
  queue_type ready_queue_;
  typedef std::unordered_map<automaton*, automaton_context*, std::hash<automaton*>, std::equal_to<automaton*>, system_allocator<std::pair<automaton* const, automaton_context*> > > context_map_type;
  context_map_type context_map_;

public:
  inline void
  add_automaton (automaton* automaton)
  {
    // Allocate a new context and insert it into the map.
    // Inserting should succeed.
    automaton_context* c = new (system_alloc ()) automaton_context ();
    std::pair<context_map_type::iterator, bool> r = context_map_.insert (std::make_pair (automaton, c));
    kassert (r.second);
  }

  inline automaton*
  current_automaton () const
  {
    return exec_context_.current_automaton ();
  }

  inline size_t
  current_value_size () const
  {
    return exec_context_.current_value_size ();
  }

  inline void
  schedule (const caction& ad)
  {
    context_map_type::iterator pos = context_map_.find (ad.automaton);
    kassert (pos != context_map_.end ());
    
    automaton_context* c = pos->second;
    
    c->push (ad);
    
    if (c->status () == NOT_SCHEDULED) {
      c->status (SCHEDULED);
      ready_queue_.push_back (c);
    }
  }
  
  inline void
  finish (bool output_status,
	  const void* buffer)
  {
    // This call won't return when executing input actions.
    exec_context_.finish_action (output_status, buffer);

    if (!ready_queue_.empty ()) {
      /* Load the execution context. */
      automaton_context* c = ready_queue_.front ();
      ready_queue_.pop_front ();
      c->status (NOT_SCHEDULED);
    
      exec_context_.load (c->front ());
      c->pop ();

      // This call doesn't not return.
      exec_context_.execute ();
    }
    else {
      /* Out of actions.  Halt. */
      exec_context_.clear ();
      asm volatile ("hlt");
    }
  }

};

#endif /* __scheduler_hpp__ */
