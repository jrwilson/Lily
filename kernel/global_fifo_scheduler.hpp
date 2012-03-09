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

  AAA - Reference counting for automata added to the scheduler.
  BBB - Reference counting for automata that add an action.
  CCC - Increment the reference count of bindings when executing an output action.
  DDD - Locking an automaton's bindings to copy.
  EEE - Lock the automaton containing the local action.
  FFF - Lock the automaton containing input actions.

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
  typedef unordered_map<automaton*, automaton_context*> context_map_type;
  static context_map_type context_map_;

  // Queue of automaton with actions to execute.
  typedef linear_set<automaton_context*> queue_type;
  static queue_type ready_queue_;

  // The action that is currently executing.
  static caction action_;

  // List of input actions to be used when executing bound output actions.
  typedef vector<binding*> input_action_list_type;
  static input_action_list_type input_action_list_;

  // Iterator that marks our progress when executing input actions.
  static input_action_list_type::const_iterator input_action_pos_;

  // Buffers produced by an output action that will be copied to the input action.
  static buffer* output_buffer_a_;
  static buffer* output_buffer_b_;

  struct sort_bindings_by_input {
    bool
    operator () (const binding* x,
		 const binding* y) const
    {
      return x->input_action ().action->automaton < y->input_action ().action->automaton;
    }
  };

  static inline void
  execute ()
  {
    kassert (action_.action != 0);
    
    automaton* a = action_.action->automaton;

    if (a->enabled ()) {
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
      vm::switch_to_directory (a->page_directory_physical_address ());
      
      uint32_t* stack_pointer = reinterpret_cast<uint32_t*> (a->stack_pointer ());
      
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
	    bda = a->buffer_create (*output_buffer_a_);
	  }
	  
	  if (output_buffer_b_ != 0) {
	    // Copy the buffer to the input automaton.
	    bdb = a->buffer_create (*output_buffer_b_);
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
    else {
      finish (false, -1, -1);
    }
  }

  static inline void
  proceed_to_input (void)
  {
    while (input_action_pos_ != input_action_list_.end ()) {
      binding* b = *input_action_pos_;
      if (b->enabled ()) {
	action_ = b->input_action ();
	// This does not return.
	execute ();
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
      binding* b = (*pos);
      // -FFF
      b->input_action ().action->automaton->unlock_execution ();
      // -CCC
      b->decref ();
    }
    input_action_list_.clear ();
  }
  
public:
  static void
  initialize ()
  {
    action_.action = 0;
  }

  static inline void
  add_automaton (automaton* a)
  {
    // +AAA
    a->incref ();
    // Allocate a new context and insert it into the map.
    // Inserting should succeed.
    automaton_context* c = new automaton_context ();
    pair<context_map_type::iterator, bool> r = context_map_.insert (make_pair (a, c));
    kassert (r.second);
  }

  static inline void
  remove_automaton (automaton* a)
  {
    context_map_type::iterator pos = context_map_.find (a);
    kassert (pos != context_map_.end ());
    context_map_.erase (pos);
    automaton_context* c = pos->second;
    ready_queue_.erase (c);
    for (automaton_context::const_iterator pos = c->begin ();
	 pos != c->end ();
	 ++pos) {
      // -BBB
      pos->action->automaton->decref ();
    }
    delete c;

    // -AAA
    a->decref ();
  }

  static inline automaton*
  current_automaton ()
  {
    kassert (action_.action != 0);
    return action_.action->automaton;
  }

  static inline void
  schedule (const caction& ad)
  {
    context_map_type::iterator pos = context_map_.find (ad.action->automaton);
    kassert (pos != context_map_.end ());
    
    automaton_context* c = pos->second;
    
    if (c->push_back (ad)) {
      // +BBB
      ad.action->automaton->incref ();
    }
    
    ready_queue_.push_back (c);
  }
  
  static inline void
  finish (bool output_fired,
	  bd_t bda,
	  bd_t bdb)
  {
    if (action_.action != 0) {
      // We were executing an action of this automaton.
      automaton* a = action_.action->automaton;

      switch (action_.action->type) {
      case INPUT:
	// We were executing an input.  Move to the next input.
	++input_action_pos_;
	proceed_to_input ();
	// -EEE
	input_action_list_.front ()->output_action ().action->automaton->unlock_execution ();
	// -BBB
	input_action_list_.front ()->output_action ().action->automaton->decref ();
	finish_output ();
	break;
      case OUTPUT:
	// We were executing an output ...
	if (output_fired) {
	  // ... and the output output did something.
	  output_buffer_a_ = a->lookup_buffer (bda);
	  // Synchronize the buffers.
	  if (output_buffer_a_ != 0) {
	    output_buffer_a_->sync (0, output_buffer_a_->size ());
	  }
	  output_buffer_b_ = a->lookup_buffer (bdb);
	  if (output_buffer_b_ != 0) {
	    output_buffer_b_->sync (0, output_buffer_b_->size ());
	  }
	  // Proceed to execute the inputs.
	  input_action_pos_ = input_action_list_.begin ();
	  // This does not return if there are inputs.
	  proceed_to_input ();
	}
	// No input actions to execute.
	// -EEE
	a->unlock_execution ();
	// -BBB
	a->decref ();
	finish_output ();
	break;
      case INTERNAL:
	// -EEE
	a->unlock_execution ();
	// -BBB
	a->decref ();
	break;
      case SYSTEM_INPUT:
	// -EEE
	a->unlock_execution ();
	// -BBB
	a->decref ();
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

    // We are done with the current action.
    action_.action = 0;

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
	c->pop_front ();

	automaton* a = action_.action->automaton;

	// The automaton exists.  Continue loading and execute.
	switch (action_.action->type) {
	case INPUT:
	  // Error.  Not a local action.
	  kassert (0);
	  break;
	case OUTPUT:
	  {
	    kassert (input_action_list_.empty ());
	    // +DDD
	    a->lock_bindings ();
	    // Copy the bindings.
	    const automaton::binding_set_type& input_actions = a->get_bound_inputs (action_);
	    for (automaton::binding_set_type::const_iterator pos = input_actions.begin ();
		 pos != input_actions.end ();
		 ++pos) {
	      input_action_list_.push_back (*pos);
	      // So they don't disappear.
	      // +CCC
	      (*pos)->incref ();
	    }
	    // -DDD
	    a->unlock_bindings ();
	    
	    // Sort the bindings by input automaton.
	    sort (input_action_list_.begin (), input_action_list_.end (), sort_bindings_by_input ());
	    
	    // We lock the automata in order.  This is called Havender's Principle.
	    bool output_locked = false;
	    for (input_action_list_type::const_iterator pos = input_action_list_.begin ();
		 pos != input_action_list_.end ();
		 ++pos) {
	      automaton* input_automaton = (*pos)->input_action ().action->automaton;
	      if (!output_locked && a < input_automaton) {
		// +EEE
		a->lock_execution ();
		output_locked = true;
	      }
	      // +FFF
	      input_automaton->lock_execution ();
	    }
	    
	    input_action_pos_ = input_action_list_.begin ();
	  }
	  break;
	case INTERNAL:
	  // +EEE
	  a->lock_execution ();
	  break;
	case SYSTEM_INPUT:
	  // Load the buffer.
	  output_buffer_a_ = action_.buffer_a;
	  output_buffer_b_ = action_.buffer_b;
	  // +EEE
	  a->lock_execution ();
	  break;
	}
	
	if (!c->empty ()) {
	  // Automaton has more actions, return to ready queue.
	  ready_queue_.push_back (c);
	}
	
	// This call does not return.
	execute ();
      }

      /* Out of actions.  Halt. */
      action_.action = 0;
      asm volatile ("sti\n"
		    "hlt\n"
		    "cli\n");
    }
  }
};

#endif /* __global_fifo_scheduler_hpp__ */
