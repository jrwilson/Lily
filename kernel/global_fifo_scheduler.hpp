#ifndef __global_fifo_scheduler_hpp__
#define __global_fifo_scheduler_hpp__

/*
  File
  ----
  global_fifo_scheduler.hpp
  
  Description
  -----------
n  Declarations for scheduling.

  TODO:  Convert this back to using iterators.

  Authors:
  Justin R. Wilson
*/

#include "action.hpp"
#include "vm.hpp"
#include "automaton.hpp"
#include "string.hpp"
#include "linear_set.hpp"
#include "deque.hpp"
#include "rts.hpp"

class global_fifo_scheduler {
private:

  // Align the stack when executing.
  static const size_t STACK_ALIGN = 16;

  // An automaton is either on the run queue (scheduled) or not.
  enum status_t {
    SCHEDULED,
    NOT_SCHEDULED
  };

  class automaton_context {
  private:
    // Flag indicating of the automaton is schedule or not.
    status_t status_;
    // Set of actions that will be executed for this automaton.
    typedef linear_set<caction, caction_hash> set_type;
    set_type set_;

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
  };

  // Map an automaton to its scheduling context.
  typedef unordered_map<aid_t, automaton_context*> context_map_type;
  static context_map_type context_map_;

  // Queue of automaton with actions to execute.
  typedef deque<automaton_context*> queue_type;
  static queue_type ready_queue_;

  // The automaton that is currently executing.
  static automaton* current_automaton_;

  // The action that is currently executing.
  static caction action_;

  // List of input actions to be used when executing bound output actions.
  typedef vector<caction> input_action_list_type;
  static input_action_list_type input_action_list_;

  // Iterator that marks our progress when executing input actions.
  static input_action_list_type::const_iterator input_action_pos_;

  // Buffers produced by an output action that will be copied to the input action.
  // TODO
  static buffer* output_buffer_a_;
  static buffer* output_buffer_b_;

  static inline void
  execute ()
  {
    kassert (current_automaton_ != 0);

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
    vm::switch_to_directory (current_automaton_->page_directory_physical_address ());
    
    uint32_t* stack_pointer = reinterpret_cast<uint32_t*> (current_automaton_->stack_pointer ());
    
    // These instructions serve a dual purpose.
    // First, they set up the cdecl calling convention for actions.
    // Second, they force the stack to be created if it is not.
    
    switch (action_.type) {
    case INPUT:
    case SYSTEM_INPUT:
      {
	bd_t bda = -1;
	bd_t bdb = -1;
	
	if (output_buffer_a_ != 0) {
	  // Copy the buffer to the input automaton.
	  bda = current_automaton_->buffer_create (*output_buffer_a_);
	}
	
	if (output_buffer_b_ != 0) {
	  // Copy the buffer to the input automaton.
	  bdb = current_automaton_->buffer_create (*output_buffer_b_);
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
    *--stack_pointer = reinterpret_cast<uint32_t> (action_.action_entry_point);
    
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
  
public:
  static void
  initialize ()
  {
    current_automaton_ = 0;
  }

  static inline void
  add_automaton (aid_t aid)
  {
    // Allocate a new context and insert it into the map.
    // Inserting should succeed.
    automaton_context* c = new automaton_context ();
    pair<context_map_type::iterator, bool> r = context_map_.insert (make_pair (aid, c));
    kassert (r.second);
  }

  static inline automaton*
  current_automaton ()
  {
    kassert (current_automaton_ != 0);
    return current_automaton_;
  }

  static inline void
  schedule (const caction& ad)
  {
    context_map_type::iterator pos = context_map_.find (ad.aid);
    kassert (pos != context_map_.end ());
    
    automaton_context* c = pos->second;
    
    c->push (ad);
    
    if (c->status () == NOT_SCHEDULED) {
      c->status (SCHEDULED);
      ready_queue_.push_back (c);
    }
  }
  
  static void
  proceed_to_input (void)
  {
    while (input_action_pos_ != input_action_list_.end ()) {
      action_ = *input_action_pos_;
      current_automaton_ = rts::lookup_automaton (action_.aid);
      if (current_automaton_ != 0) {
	// This does not return.
	execute ();
      }
      else {
	++input_action_pos_;
      }
    }
  }

  static inline void
  finish (bool output_fired,
	  bd_t bda,
	  bd_t bdb)
  {
    if (current_automaton_ != 0) {
      // We were executing an action.
      switch (action_.type) {
      case INPUT:
	// We were executing an input.  Move to the next input.
	++input_action_pos_;
	proceed_to_input ();
	break;
      case OUTPUT:
	// We were executing an output ...
	if (output_fired) {
	  // ... and the output output did something.
	  output_buffer_a_ = current_automaton_->lookup_buffer (bda);
	  // Synchronize the buffers.
	  if (output_buffer_a_ != 0) {
	    output_buffer_a_->sync (0, output_buffer_a_->size ());
	  }
	  output_buffer_b_ = current_automaton_->lookup_buffer (bdb);
	  if (output_buffer_b_ != 0) {
	    output_buffer_b_->sync (0, output_buffer_b_->size ());
	  }
	  if (!input_action_list_.empty ()) {
	    // Proceed to execute the inputs.
	    input_action_pos_ = input_action_list_.begin ();
	    proceed_to_input ();
	    // This does not return.
	    execute ();
	  }
	  // There were no inputs.
	  output_buffer_a_ = 0;
	  output_buffer_b_ = 0;
	}
	break;
      case INTERNAL:
	// Do nothing.
	break;
      case SYSTEM_INPUT:
	// Destroy the buffers.
	if (output_buffer_a_ != 0) {
	  delete output_buffer_a_;
	  output_buffer_a_ = 0;
	}
	if (output_buffer_b_ != 0) {
	  delete output_buffer_b_;
	  output_buffer_b_ = 0;
	}
	break;
      }
    }

    // We are done with the current automaton.
    current_automaton_ = 0;

    // Check for interrupts.
    asm volatile ("sti\n"
		  "nop\n"
		  "cli\n");

    for (;;) {

      while (!ready_queue_.empty ()) {
	// Get the automaton context and remove it from the ready queue.
	automaton_context* c = ready_queue_.front ();
	ready_queue_.pop_front ();
	
	// Load the action.
	action_ = c->front ();
	c->pop ();
	current_automaton_ = rts::lookup_automaton (action_.aid);

	if (current_automaton_ != 0) {
	  // The automaton exists.  Continue loading and execute.
	  switch (action_.type) {
	  case INPUT:
	    // Do nothing.
	    break;
	  case OUTPUT:
	    {
	      // Lookup the input actions.
	      input_action_list_.clear ();
	      const automaton::input_action_set_type* input_actions = current_automaton_->get_bound_inputs (action_);
	      if (input_actions != 0) {
		for (automaton::input_action_set_type::const_iterator pos = input_actions->begin ();
		     pos != input_actions->end ();
		     ++pos) {
		  input_action_list_.push_back (pos->input);
		}
	      }
	      input_action_pos_ = input_action_list_.begin ();
	    }
	    break;
	  case INTERNAL:
	    // Do nothing.
	    break;
	  case SYSTEM_INPUT:
	    // Load the buffer.
	    output_buffer_a_ = action_.buffer_a;
	    output_buffer_b_ = action_.buffer_b;
	    break;
	  }
	  
	  if (!c->empty ()) {
	    // Automaton has more actions, return to ready queue.
	    ready_queue_.push_back (c);
	  }
	  else {
	    // Leave out.
	    c->status (NOT_SCHEDULED);
	  }
	  
	  // This call does not return.
	  execute ();
	}
      }

      /* Out of actions.  Halt. */
      kassert (current_automaton_ == 0);
      asm volatile ("sti\n"
		    "hlt\n"
		    "cli\n");
    }
  }
};

#endif /* __global_fifo_scheduler_hpp__ */
