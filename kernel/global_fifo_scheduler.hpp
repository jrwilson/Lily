#ifndef __global_fifo_scheduler_hpp__
#define __global_fifo_scheduler_hpp__

/*
  File
  ----
  global_fifo_scheduler.hpp
  
  Description
  -----------
  Declarations for scheduling.

  Authors:
  Justin R. Wilson
*/

#include "action.hpp"
#include <deque>
#include <unordered_set>
#include "vm.hpp"
#include "automaton.hpp"

class global_fifo_scheduler {
private:
  enum status_t {
    SCHEDULED,
    NOT_SCHEDULED
  };

  // Align the stack when switching executing.
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

    inline bool
    empty () const
    {
      return queue_.empty ();
    }

  private:
    status_t status_;
    typedef std::deque<caction, kernel_allocator<caction> > queue_type;
    queue_type queue_;
    typedef std::unordered_set<caction, caction_hash, std::equal_to<caction>, kernel_allocator<caction> > set_type;
    set_type set_;
  };
  
  class execution_context {
  private:
    uint8_t value_buffer_[MAX_COPY_VALUE_SIZE];
    caction action_;
    const automaton::input_action_set_type* input_actions_;
    automaton::input_action_set_type::const_iterator input_action_pos_;
    buffer* output_buffer_;
    bid_t input_buffer_;

  public:
    execution_context ()
    {
      clear ();
    }

    inline void
    clear ()
    {
      action_.action = 0;
      action_.parameter = 0;
    }

    inline const caction&
    current_action () const
    {
      kassert (action_.action != 0);
      return action_;
    }

    inline void
    load (const caction& ad)
    {
      action_ = ad;
    }

    inline void
    finish_action (bool status,
		   bid_t bid,
		   const void* buffer)
    {
      if (action_.action != 0) {	
	switch (action_.action->type) {
	case INPUT:
	  if (action_.action->buffer_value_mode == BUFFER_VALUE) {
	    // Destroy the buffer.
	    action_.action->automaton->buffer_destroy (input_buffer_);
	  }
	  /* Move to the next input. */
	  ++input_action_pos_;
	  if (input_action_pos_ != input_actions_->end ()) {
	    /* Load the execution context. */
	    action_ = input_action_pos_->input;
	    /* Execute.  (This does not return). */
	    execute ();
	  }
	  if (action_.action->buffer_value_mode == BUFFER_VALUE) {
	    // Destroy the buffer.
	    destroy (output_buffer_, kernel_alloc ());
	  }
	  break;
	case OUTPUT:
	  // Only proceed if the output executed.
	  // If the output should have produced data (value_size_ != 0), then check that the supplied buffer is valid.
	  // Finally, the the inputs bound to the output.  If an input exists, proceed with execution.
	  if (status) {
	    if (action_.action->buffer_value_mode == BUFFER_VALUE) {
	      // The output action produced a buffer.  Remove it from the automaton.
	      output_buffer_ = action_.action->automaton->buffer_output_destroy (bid);
	    }

	    input_actions_ = action_.action->automaton->get_bound_inputs (action_);
	    if (input_actions_ != 0) {
	      if (action_.action->copy_value_mode == COPY_VALUE) {
		// Copy the value.
		memcpy (value_buffer_, buffer, action_.action->copy_value_size);
	      }
	      /* Load the execution context. */
	      input_action_pos_ = input_actions_->begin ();
	      action_ = input_action_pos_->input;
	      /* Execute.  (This does not return). */
	      execute ();
	    }
	    if (action_.action->buffer_value_mode == BUFFER_VALUE) {
	      // Destroy the buffer.
	      destroy (output_buffer_, kernel_alloc ());
	    }
	  }
	  break;
	case INTERNAL:
	  break;
	}
      }
    }

    inline void
    execute ()
    {
      // Switch page directories.
      vm::switch_to_directory (action_.action->automaton->page_directory_frame ());

      uint32_t* stack_pointer = reinterpret_cast<uint32_t*> (action_.action->automaton->stack_pointer ());

      // These instructions serve a dual purpose.
      // First, they set up the cdecl calling convention for actions.
      // Second, they force the stack to be created if it is not.

      if (action_.action->type == INPUT) {
	if (action_.action->copy_value_mode == COPY_VALUE) {
	  // Copy the value to the stack.
	  stack_pointer = static_cast<uint32_t*> (const_cast<void*> (align_down (reinterpret_cast<uint8_t*> (stack_pointer) - action_.action->copy_value_size, STACK_ALIGN)));
	  memcpy (stack_pointer, value_buffer_, action_.action->copy_value_size);
	}
	if (action_.action->buffer_value_mode == BUFFER_VALUE) {
	  // Copy the buffer to the input automaton.
	  input_buffer_ = action_.action->automaton->buffer_create (*output_buffer_);
	  *--stack_pointer = input_buffer_;
	}
      }
      
      switch (action_.action->parameter_mode) {
      case NO_PARAMETER:
      	break;
      case PARAMETER:
      case AUTO_PARAMETER:
	// Copy the parameter to the stack.
	*--stack_pointer = static_cast<uint32_t> (action_.parameter);
	break;
      }

      // Push a bogus instruction pointer so we can use the cdecl calling convention.
      *--stack_pointer = 0;
      uint32_t* sp = stack_pointer;

      // Push the stack segment.
      *--stack_pointer = gdt::USER_DATA_SELECTOR | descriptor::RING3;
      
      // Push the stack pointer.
      *--stack_pointer = reinterpret_cast<uint32_t> (sp);

      // Push the flags.
      uint32_t eflags;
      asm ("pushf\n"
      	   "pop %0\n" : "=g"(eflags) : :);
      *--stack_pointer = eflags;

      // Push the code segment.
      *--stack_pointer = gdt::USER_CODE_SELECTOR | descriptor::RING3;

      // Push the instruction pointer.
      *--stack_pointer = reinterpret_cast<uint32_t> (action_.action->action_entry_point);
      
      asm ("mov %0, %%ds\n"	// Load the data segments.
	   "mov %0, %%es\n"	// Load the data segments.
	   "mov %0, %%fs\n"	// Load the data segments.
	   "mov %0, %%gs\n"	// Load the data segments.
	   "mov %1, %%esp\n"	// Load the new stack pointer.
      	   "xor %%eax, %%eax\n"	// Clear the registers.
      	   "xor %%ebx, %%ebx\n"
      	   "xor %%ecx, %%ecx\n"
      	   "xor %%edx, %%edx\n"
      	   "xor %%edi, %%edi\n"
      	   "xor %%esi, %%esi\n"
      	   "xor %%ebp, %%ebp\n"
      	   "iret\n" :: "r"(gdt::USER_DATA_SELECTOR | descriptor::RING3), "r"(stack_pointer));
    }
  };

  /* TODO:  Need one per core. */
  static execution_context exec_context_;
  typedef std::deque<automaton_context*, kernel_allocator<automaton_context*> > queue_type;
  static queue_type ready_queue_;
  typedef std::unordered_map<automaton*, automaton_context*, std::hash<automaton*>, std::equal_to<automaton*>, kernel_allocator<std::pair<automaton* const, automaton_context*> > > context_map_type;
  static context_map_type context_map_;

public:
  static void
  add_automaton (automaton* automaton);

  static inline const caction&
  current_action ()
  {
    return exec_context_.current_action ();
  }

  static inline void
  schedule (const caction& ad)
  {
    context_map_type::iterator pos = context_map_.find (ad.action->automaton);
    kassert (pos != context_map_.end ());
    
    automaton_context* c = pos->second;
    
    c->push (ad);
    
    if (c->status () == NOT_SCHEDULED) {
      c->status (SCHEDULED);
      ready_queue_.push_back (c);
    }
  }
  
  static inline void
  finish (bool status,
	  bid_t bid,
	  const void* buffer)
  {
    // This call won't return when executing input actions.
    exec_context_.finish_action (status, bid, buffer);

    if (!ready_queue_.empty ()) {
      // Get the automaton context and remove it from the ready queue.
      automaton_context* c = ready_queue_.front ();
      ready_queue_.pop_front ();
    
      // Load the execution context and remove it from the automaton context.
      exec_context_.load (c->front ());
      c->pop ();

      if (!c->empty ()) {
	// Automaton has more actions, return to ready queue.
	ready_queue_.push_back (c);
      }
      else {
	// Leave out.
	c->status (NOT_SCHEDULED);
      }

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

#endif /* __global_fifo_scheduler_hpp__ */
