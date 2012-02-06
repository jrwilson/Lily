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
#include "vm.hpp"
#include "automaton.hpp"
#include "string.hpp"
#include "linear_set.hpp"
#include "deque.hpp"

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
      set_.push_back (ap);
    }

    inline void
    pop ()
    {
      set_.pop_front ();
    }

    inline const caction&
    front () const
    {
      return set_.front ();
    }

    inline bool
    empty () const
    {
      return set_.empty ();
    }

  private:
    status_t status_;
    typedef linear_set<caction, caction_hash> set_type;
    set_type set_;
  };
  
  class execution_context {
  private:
    caction action_;
    const automaton::input_action_set_type* input_actions_;
    automaton::input_action_set_type::const_iterator input_action_pos_;
    buffer* output_buffer_;
    int flags_;

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
      output_buffer_ = 0;
      flags_ = 0;
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
      switch (action_.action->type) {
      case INPUT:
	// Do nothing.
	break;
      case OUTPUT:
	// Lookup the input actions.
	input_actions_ = action_.action->automaton->get_bound_inputs (action_);
	break;
      case INTERNAL:
	// Do nothing.
	break;
      case SYSTEM_INPUT:
	// Load the buffer.
	output_buffer_ = action_.system_input_buffer;
	// Destroy it when finished.
	if (output_buffer_ != 0) {
	  flags_ = LILY_SYSCALL_FINISH_DESTROY;
	}
	else {
	  flags_ = 0;
	}
	break;
      }
    }

    inline void
    finish_action (bd_t bd,
		   int flags)
    {
      if (action_.action != 0) {	
	switch (action_.action->type) {
	case INPUT:
	  /* Move to the next input. */
	  ++input_action_pos_;
	  if (input_action_pos_ != input_actions_->end ()) {
	    /* Load the execution context. */
	    action_ = input_action_pos_->input;
	    /* Execute.  (This does not return). */
	    execute ();
	  }
	  // Fall through.
	case SYSTEM_INPUT:
	  // Finished executing input actions.
	  if (flags_ == LILY_SYSCALL_FINISH_DESTROY) {
	    // The buffer must exist.
	    kassert (output_buffer_ != 0);
	    // Destroy the buffer.
	    delete output_buffer_;
	    output_buffer_ = 0;
	  }
	  break;
	case OUTPUT:
	  if (flags == LILY_SYSCALL_FINISH_YES || flags == LILY_SYSCALL_FINISH_DESTROY) {
	    // The output did something.
	    if (flags == LILY_SYSCALL_FINISH_YES) {
	      output_buffer_ = action_.action->automaton->lookup_buffer (bd);
	    }
	    else {
	      output_buffer_ = action_.action->automaton->buffer_output_destroy (bd);
	    }
	    flags_ = flags;
	    if (input_actions_ != 0) {
	      /* Load the execution context. */
	      input_action_pos_ = input_actions_->begin ();
	      load (input_action_pos_->input);
	      /* Execute.  (This does not return). */
	      execute ();
	    }
	    // There were no inputs.
	    if (flags_ == LILY_SYSCALL_FINISH_DESTROY) {
	      // The buffer must exist.
	      kassert (output_buffer_ != 0);
	      // Destroy the buffer.
	      delete output_buffer_;
	      output_buffer_ = 0;
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
      // switch (action_.action->type) {
      // case INPUT:
      // 	kout << "?";
      // 	break;
      // case OUTPUT:
      // 	kout << "!";
      // 	break;
      // case INTERNAL:
      // 	kout << "#";
      // 	break;
      // case SYSTEM_INPUT:
      // 	kout << "*";
      // 	break;
      // }
      // kout << "\t" << action_.action->automaton->aid () << "\t" << action_.action->action_number << "\t" << action_.parameter << endl;

      // Switch page directories.
      vm::switch_to_directory (action_.action->automaton->page_directory_physical_address ());

      uint32_t* stack_pointer = reinterpret_cast<uint32_t*> (action_.action->automaton->stack_pointer ());

      // These instructions serve a dual purpose.
      // First, they set up the cdecl calling convention for actions.
      // Second, they force the stack to be created if it is not.

      switch (action_.action->type) {
      case INPUT:
      case SYSTEM_INPUT:
	{
	  bd_t input_buffer;
	  size_t buffer_capacity;
	  const void* buf;
	  
	  if (output_buffer_ != 0) {
	    // Copy the buffer to the input automaton and try to map it.
	    input_buffer = action_.action->automaton->buffer_create (*output_buffer_);
	    buffer_capacity = output_buffer_->capacity ();
	    buf = action_.action->automaton->buffer_map (input_buffer).first;
	  }
	  else {
	    input_buffer = -1;
	    buffer_capacity = 0;
	    buf = 0;
	  }
	  
	  // Push the address.
	  *--stack_pointer = reinterpret_cast<uint32_t> (buf);
	  // Push the buffer capacity.
	  *--stack_pointer = buffer_capacity;
	  // Push the buffer.
	  *--stack_pointer = input_buffer;	
	}
	break;
      case OUTPUT:
	{
	  // Push the binding count.
	  if (input_actions_ != 0) {
	    *--stack_pointer = input_actions_->size ();
	  }
	  else {
	    *--stack_pointer = 0;
	  }
	}
	break;
      case INTERNAL:
	// Do nothing.
	break;
      }
      
      // Push the parameter.
      *--stack_pointer = action_.parameter;

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
  typedef deque<automaton_context*> queue_type;
  static queue_type ready_queue_;
  typedef unordered_map<automaton*, automaton_context*> context_map_type;
  static context_map_type context_map_;

public:
  static inline void
  add_automaton (automaton* automaton)
  {
    // Allocate a new context and insert it into the map.
    // Inserting should succeed.
    automaton_context* c = new automaton_context ();
    pair<context_map_type::iterator, bool> r = context_map_.insert (make_pair (automaton, c));
    kassert (r.second);
  }

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
  finish (bd_t bd,
	  int flags)
  {
    // This call won't return when executing input actions.
    exec_context_.finish_action (bd, flags);

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
