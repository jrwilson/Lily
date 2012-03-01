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
    buffer* output_buffer_a_;
    buffer* output_buffer_b_;
    bool destroy_buffer_;

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
      output_buffer_a_ = 0;
      output_buffer_b_ = 0;
      destroy_buffer_ = false;
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
	output_buffer_a_ = action_.system_input_buffer_a;
	output_buffer_b_ = action_.system_input_buffer_b;
	destroy_buffer_ = true;
	break;
      }
    }

    inline void
    finish_action (bool output_fired,
		   bd_t bda,
		   bd_t bdb)

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
	  if (output_buffer_a_ != 0 && destroy_buffer_) {
	    delete output_buffer_a_;
	    output_buffer_a_ = 0;
	  }
	  if (output_buffer_b_ != 0 && destroy_buffer_) {
	    delete output_buffer_b_;
	    output_buffer_b_ = 0;
	  }
	  destroy_buffer_ = false;
	  break;
	case OUTPUT:
	  if (output_fired) {
	    // The output did something.
	    output_buffer_a_ = action_.action->automaton->lookup_buffer (bda);
	    // Synchronize.
	    if (output_buffer_a_ != 0) {
	      output_buffer_a_->sync (0, output_buffer_a_->size ());
	    }
	    // The output did something.
	    output_buffer_b_ = action_.action->automaton->lookup_buffer (bdb);
	    // Synchronize.
	    if (output_buffer_b_ != 0) {
	      output_buffer_b_->sync (0, output_buffer_b_->size ());
	    }
	    destroy_buffer_ = false;
	    if (input_actions_ != 0) {
	      /* Load the execution context. */
	      input_action_pos_ = input_actions_->begin ();
	      load (input_action_pos_->input);
	      /* Execute.  (This does not return). */
	      execute ();
	    }
	    // There were no inputs.
	    output_buffer_a_ = 0;
	    output_buffer_b_ = 0;
	    destroy_buffer_ = false;
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
	  bd_t bda = -1;
	  bd_t bdb = -1;
	  
	  if (output_buffer_a_ != 0) {
	    // Copy the buffer to the input automaton.
	    bda = action_.action->automaton->buffer_create (*output_buffer_a_);
	  }

	  if (output_buffer_b_ != 0) {
	    // Copy the buffer to the input automaton.
	    bdb = action_.action->automaton->buffer_create (*output_buffer_b_);
	  }

	  // Push the buffers.
	  *--stack_pointer = bdb;
	  *--stack_pointer = bda;
	}
	break;
      case OUTPUT:
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
  finish (bool output_fired,
	  bd_t bda,
	  bd_t bdb)
  {
    // This call won't return when executing input actions.
    exec_context_.finish_action (output_fired, bda, bdb);

    // Check for interrupts.
    asm volatile ("sti\n"
		  "nop\n"
		  "cli\n");

    for (;;) {
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

      /* Out of actions.  Halt. */
      exec_context_.clear ();
      asm volatile ("sti\n"
		    "hlt\n"
		    "cli\n");
    }
  }

};

#endif /* __global_fifo_scheduler_hpp__ */
