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

#include "binding_manager.hpp"
#include "action_type.hpp"
#include <deque>

class scheduler {
private:
  enum status_t {
    SCHEDULED,
    NOT_SCHEDULED
  };

  struct automaton_context {
    automaton_interface* automaton_;
    void* action_entry_point;
    parameter_t parameter;
    status_t status;
    
    automaton_context (automaton_interface* a) :
      automaton_ (a),
      status (NOT_SCHEDULED)
    { }
  };
    
  struct execution_context {
    logical_address switch_stack;
    size_t switch_stack_size;
    action_type_t action_type;
    automaton_interface* current_automaton;
    void* current_action_entry_point;
    parameter_t current_parameter;
    const binding_manager::input_action_set_type* input_actions;
    binding_manager::input_action_set_type::const_iterator input_action_pos;
    value_t output_value;
  };

  typedef std::deque<automaton_context*> queue_type;
  static queue_type* ready_queue_;
  /* TODO:  Need one per core. */
  static execution_context exec_context;

  static void
  switch_to_next_action ();

public:
  typedef automaton_context automaton_context_type;

  static void
  initialize (automaton_interface* automaton);
  
  static void
  set_switch_stack (logical_address switch_stack,
		    size_t switch_stack_size);
  
  static automaton_context*
  allocate_context (automaton_interface* automaton);
  
  static automaton_interface*
  get_current_automaton (void);
  
  static void
  schedule_action (automaton_interface* automaton,
		   void* action_entry_point,
		   parameter_t parameter);
  
  static void
  finish_action (int output_status,
		 value_t value);
};

#endif /* __scheduler_hpp__ */
