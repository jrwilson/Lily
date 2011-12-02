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

class list_alloc;

// Size of the stack to use when switching between automata.
// Note that this is probably not the number of bytes as each element in the array may occupy more than one byte.
static const size_t SWITCH_STACK_SIZE = 256;

class scheduler {
private:
  enum status_t {
    SCHEDULED,
    NOT_SCHEDULED
  };

  struct automaton_context {
    ::action action;
    status_t status;
    
    automaton_context (::automaton* a) :
      status (NOT_SCHEDULED)
    {
      action.automaton = a;
    }
  };
  
  class execution_context {
  private:
    binding_manager& binding_manager_;
    uint32_t switch_stack_[SWITCH_STACK_SIZE];
    action current_action_;
    action_type_t action_type_;
    const binding_manager::input_action_set_type* input_actions_;
    binding_manager::input_action_set_type::const_iterator input_action_pos_;
    value_t output_value_;
    
  public:
    execution_context (binding_manager& bm);

    void
    clear ();

    ::automaton*
    automaton () const;

    action_type_t
    action_type () const;

    void
    load (automaton_context* c);

    void
    execute () const;

    void
    finish_action (bool output_status,
		   value_t output_value);
  };

  list_alloc& alloc_;
  /* TODO:  Need one per core. */
  execution_context exec_context_;
  typedef std::deque<automaton_context*, list_allocator<automaton_context*> > queue_type;
  queue_type ready_queue_;
  typedef std::unordered_map<automaton*, automaton_context*, std::hash<automaton*>, std::equal_to<automaton*>, list_allocator<std::pair<automaton* const, automaton_context*> > > context_map_type;
  context_map_type context_map_;

  void
  switch_to_next_action ();

public:
  scheduler (list_alloc& a,
	     binding_manager& bm);

  void
  add_automaton (automaton* automaton);

  automaton*
  current_automaton () const;

  void
  schedule_action (automaton* automaton,
		   void* action_entry_point,
		   parameter_t parameter);
  
  void
  finish_action (bool output_status,
		 value_t output_value);

};

#endif /* __scheduler_hpp__ */
