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
    status_t status;
    ::automaton* automaton;
    size_t action_entry_point;
    void* parameter;

    
    automaton_context (::automaton* a) :
      status (NOT_SCHEDULED),
      automaton (a)
    { }
  };
  
  class execution_context {
  private:
    binding_manager& binding_manager_;
    uint32_t switch_stack_[SWITCH_STACK_SIZE];
    uint8_t message_buffer_[MESSAGE_BUFFER_SIZE];

    automaton* automaton_;
    automaton::action action_;
    void* parameter_;

    const binding_manager::input_action_set_type* input_actions_;
    binding_manager::input_action_set_type::const_iterator input_action_pos_;

    void
    exec (int end_of_stack = -1) const;
    
  public:
    execution_context (binding_manager& bm);

    void
    clear ();

    automaton*
    current_automaton () const;

    void
    load (automaton_context* c);

    void
    finish_action (void* buffer);

    void
    execute () const;
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
		   size_t action_entry_point,
		   void* parameter);
  
  void
  finish_action (size_t action_entry_point,
		 void* parameter,
		 void* buffer);
};

#endif /* __scheduler_hpp__ */
