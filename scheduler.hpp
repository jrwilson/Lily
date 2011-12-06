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

template <class Alloc, template <typename> class Allocator>
class scheduler {
private:
  typedef automaton<Alloc, Allocator> automaton_type;

  enum status_t {
    SCHEDULED,
    NOT_SCHEDULED
  };

  struct automaton_context {
    status_t status;
    automaton_type* automaton;
    size_t action_entry_point;
    void* parameter;

    
    automaton_context (automaton_type* a) :
      status (NOT_SCHEDULED),
      automaton (a)
    { }
  };
  
  class execution_context {
  private:
    binding_manager<Alloc, Allocator>& binding_manager_;
    uint32_t switch_stack_[SWITCH_STACK_SIZE];
    uint8_t value_buffer_[VALUE_BUFFER_SIZE];

    automaton_type* automaton_;
    typename automaton_type::action action_;
    const void* parameter_;

    const typename binding_manager<Alloc, Allocator>::input_action_set_type* input_actions_;
    typename binding_manager<Alloc, Allocator>::input_action_set_type::const_iterator input_action_pos_;

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
      asm volatile ("mov %%esp, %0\n" : "=m"(stack_begin));

      // Determine the end of the stack.
      stack_end = &end_of_stack + 1;

      // Compute the size of the stack.
      stack_size = stack_end - stack_begin;

      new_stack_begin = static_cast<uint32_t*> (const_cast<void*> (align_down (switch_stack_ + SWITCH_STACK_SIZE - stack_size, STACK_ALIGN)));

      // Check that the stack will work.
      kassert (new_stack_begin >= switch_stack_);

      //
      for (idx = 0; idx < stack_size; ++idx) {
	new_stack_begin[idx] = stack_begin[idx];
      }
  
      /* Update the base and stack pointers. */
      asm volatile ("add %0, %%esp\n"
		    "add %0, %%ebp\n" :: "r"((new_stack_begin - stack_begin) * sizeof (uint32_t)) : "%esp", "memory");

      /* Switch page directories. */
      vm_manager::switch_to_directory (automaton_->page_directory ());

      switch (action_.type) {
      case automaton_type::INPUT:
	// Copy the value to the stack.
	stack_pointer = static_cast<uint32_t*> (const_cast<void*> (align_down (reinterpret_cast<uint8_t*> (stack_pointer) - action_.value_size, STACK_ALIGN)));
	memcpy (stack_pointer, value_buffer_, action_.value_size);
	if (action_.is_parameterized) {
	  // Copy the parameter to the stack.
	  --stack_pointer;
	  *stack_pointer = reinterpret_cast<uint32_t> (parameter_);
	}
	asm volatile ("mov %0, %%eax\n"	// Load the new stack segment.
		      "mov %%ax, %%ss\n"
		      "mov %1, %%eax\n"	// Load the new stack pointer.
		      "mov %%eax, %%esp\n"
		      "pushl $0x0\n"	// Push a bogus instruction pointer so we can use the cdecl calling convention.
		      "pushf\n"		// Push flags (EFLAGS).
		      "pushl %2\n"		// Push the code segment (CS).
		      "pushl %3\n"		// Push the instruction pointer (EIP).
		      "iret\n" :: "m"(stack_segment), "m"(stack_pointer), "m"(code_segment), "m"(action_.action_entry_point) : "%eax");
	break;
      case automaton_type::OUTPUT:
      case automaton_type::INTERNAL:
	if (action_.is_parameterized) {
	  // Copy the parameter to the stack.
	  --stack_pointer;
	  *stack_pointer = reinterpret_cast<uint32_t> (parameter_);
	}
	asm volatile ("mov %0, %%eax\n"	// Load the new stack segment.
		      "mov %%ax, %%ss\n"
		      "mov %1, %%eax\n"	// Load the new stack pointer.
		      "mov %%eax, %%esp\n"
		      "pushl $0x0\n"	// Push a bogus instruction pointer so we can use the cdecl calling convention.
		      "pushf\n"		// Push flags (EFLAGS).
		      "pushl %2\n"		// Push the code segment (CS).
		      "pushl %3\n"		// Push the instruction pointer (EIP).
		      "iret\n" :: "m"(stack_segment), "m"(stack_pointer), "m"(code_segment), "m"(action_.action_entry_point) : "%eax", "%esp");
	break;
      case automaton_type::NO_ACTION:
	// Error.
	kassert (0);
	break;
      }
    }
    
  public:
    execution_context (binding_manager<Alloc, Allocator>& bm ) :
      binding_manager_ (bm)
    {
      clear ();
    }

    void
    clear ()
    {
      automaton_ = 0;
      action_ = typename automaton_type::action ();
      parameter_ = 0;
    }

    automaton_type*
    current_automaton () const
    {
      kassert (automaton_ != 0);
      return automaton_;
    }

    void
    load (automaton_context* c)
    {
      automaton_ = c->automaton;
      action_ = automaton_->get_action (c->action_entry_point);
      parameter_ = c->parameter;
      c->status = NOT_SCHEDULED;
    }

    void
    finish_action (void* buffer)
    {
      switch (action_.type) {
      case automaton_type::INPUT:
	/* Move to the next input. */
	++input_action_pos_;
	if (input_action_pos_ != input_actions_->end ()) {
	  /* Load the execution context. */
	  automaton_ = input_action_pos_->automaton;
	  action_.action_entry_point = input_action_pos_->action_entry_point;
	  parameter_ = input_action_pos_->parameter;
	  /* Execute.  (This does not return). */
	  exec ();
	}
	break;
      case automaton_type::OUTPUT:
	if (buffer != 0) {
	  if (automaton_->verify_span (buffer, action_.value_size)) {
	    input_actions_ = binding_manager_.get_bound_inputs (automaton_, action_.action_entry_point, parameter_);
	    if (input_actions_ != 0) {
	      /* The output executed and there are inputs. */
	      /* Load the execution context. */
	      input_action_pos_ = input_actions_->begin ();
	      automaton_ = input_action_pos_->automaton;
	      action_.action_entry_point = input_action_pos_->action_entry_point;
	      parameter_ = input_action_pos_->parameter;
	      // We know its an input.
	      action_.type = automaton_type::INPUT;
	      // Copy the value.
	      memcpy (value_buffer_, buffer, action_.value_size);
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
      case automaton_type::INTERNAL:
      case automaton_type::NO_ACTION:
	break;
      }
    }

    void
    execute () const
    {
      switch (action_.type) {
      case automaton_type::OUTPUT:
      case automaton_type::INTERNAL:
	exec ();
	break;
      case automaton_type::NO_ACTION:
      case automaton_type::INPUT:
	break;
      }
    }

  };

  Alloc& alloc_;
  /* TODO:  Need one per core. */
  execution_context exec_context_;
  typedef std::deque<automaton_context*, Allocator<automaton_context*> > queue_type;
  queue_type ready_queue_;
  typedef std::unordered_map<automaton_type*, automaton_context*, std::hash<automaton_type*>, std::equal_to<automaton_type*>, Allocator<std::pair<automaton_type* const, automaton_context*> > > context_map_type;
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
  scheduler (Alloc& a,
	     binding_manager<Alloc, Allocator>& bm)  :
    alloc_ (a),
    exec_context_ (bm),
    ready_queue_ (typename queue_type::allocator_type (a)),
    context_map_ (3, typename context_map_type::hasher (), typename context_map_type::key_equal (), typename context_map_type::allocator_type (a))
  { }

  void
  add_automaton (automaton_type* automaton)
  {
    // Allocate a new context and insert it into the map.
    // Inserting should succeed.
    automaton_context* c = new (alloc_) automaton_context (automaton);
    std::pair<typename context_map_type::iterator, bool> r = context_map_.insert (std::make_pair (automaton, c));
    kassert (r.second);
  }

  automaton_type*
  current_automaton () const
  {
    return exec_context_.current_automaton ();
  }

  void
  schedule_action (automaton_type* automaton,
		   size_t action_entry_point,
		   void* parameter)
  {
    typename context_map_type::iterator pos = context_map_.find (automaton);
    kassert (pos != context_map_.end ());

    automaton_context* c = pos->second;

    c->action_entry_point = action_entry_point;
    c->parameter = parameter;
    
    if (c->status == NOT_SCHEDULED) {
      c->status = SCHEDULED;
      ready_queue_.push_back (c);
    }
  }
  
  void
  finish_action (size_t action_entry_point,
		 void* parameter,
		 void* buffer)
  {
    if (action_entry_point != 0) {
      schedule_action (current_automaton (), action_entry_point, parameter);
    }
    exec_context_.finish_action (buffer);
    switch_to_next_action ();
  }

};

#endif /* __scheduler_hpp__ */
