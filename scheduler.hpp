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
#include <utility>
#include "automaton_interface.hpp"
#include "interrupts.hpp"

class scheduler {
private:
  automaton_interface* current_automaton_;

public:
  scheduler (automaton_interface* current_automaton) :
    current_automaton_ (current_automaton)
  { }

  inline automaton_interface*
  current_automaton () const
  {
    return current_automaton_;
  }
};

// using namespace std::rel_ops;

// template <typename AllocatorTag, template <typename> class Allocator>
// class scheduler {
// private:
//   enum status_t {
//     SCHEDULED,
//     NOT_SCHEDULED
//   };

//   struct automaton_context {
//     automaton_interface* automaton_;
//     void* action_entry_point;
//     parameter_t parameter;
//     status_t status;
    
//     automaton_context (automaton_interface* a) :
//       automaton_ (a),
//       status (NOT_SCHEDULED)
//     { }
//   };

//   struct execution_context {
//     logical_address switch_stack;
//     size_t switch_stack_size;
//     action_type_t action_type;
//     automaton_interface* current_automaton;
//     void* current_action_entry_point;
//     parameter_t current_parameter;
//     const typename binding_manager<AllocatorTag, Allocator>::input_action_set_type* input_actions;
//     typename binding_manager<AllocatorTag, Allocator>::input_action_set_type::const_iterator input_action_pos;
//     value_t output_value;
//   };

//   typedef std::deque<automaton_context*, Allocator<automaton_context*> > queue_type;
//   static queue_type* ready_queue_;
//   /* TODO:  Need one per core. */
//   static execution_context exec_context;

//   static void
//   switch_to_next_action ()
//   {
//     if (!ready_queue_->empty ()) {
//       /* Load the execution context. */
//       automaton_context* ptr = ready_queue_->front ();
//       ready_queue_->pop_front ();
//       ptr->status = NOT_SCHEDULED;
//       exec_context.current_automaton = ptr->automaton_;
//       exec_context.current_action_entry_point = ptr->action_entry_point;
//       exec_context.current_parameter = ptr->parameter;
      
//       /* Get the type to make sure its a local action. */
//       exec_context.action_type = exec_context.current_automaton->get_action_type (exec_context.current_action_entry_point);
//       switch (exec_context.action_type) {
//       case OUTPUT:
//       case INTERNAL:
// 	/* Execute.  (This does not return). */
// 	exec_context.current_automaton->execute (exec_context.switch_stack, exec_context.switch_stack_size, exec_context.current_action_entry_point, exec_context.current_parameter, 0);
// 	break;
//       case INPUT:
//       case NO_ACTION:
// 	/* TODO:  Kill the automaton for scheduling a bad action. */
// 	kassert (0);
// 	/* Recur to get the next. */
// 	switch_to_next_action ();
// 	break;
//       }
//     }
//     else {
//       /* Out of actions.  Halt. */
//       exec_context.action_type = NO_ACTION;
//       interrupts::enable ();
//       asm volatile ("hlt");
//     }
//   }
  
// public:
//   static void
//   initialize (automaton_interface* automaton)
//   {
//     kassert (automaton != 0);
    
//     exec_context.action_type = NO_ACTION;
//     exec_context.current_automaton = automaton;
    
//     ready_queue_ = new (AllocatorTag ()) queue_type ();
//   }
  
//   static void
//   set_switch_stack (logical_address switch_stack,
// 		    size_t switch_stack_size)
//   {
//     kassert (switch_stack != logical_address ());
//     kassert (switch_stack >= KERNEL_VIRTUAL_BASE);
//     kassert (switch_stack_size > 0);
    
//     exec_context.switch_stack = switch_stack;
//     exec_context.switch_stack_size = switch_stack_size;
//   }
  
//   static automaton_context*
//   allocate_context (automaton_interface* automaton)
//   {
//     kassert (automaton != 0);
//     return new (AllocatorTag ()) automaton_context (automaton);
//   }
  
//   static automaton_interface*
//   get_current_automaton (void)
//   {
//     kassert (exec_context.current_automaton != 0);
//     return exec_context.current_automaton;
//   }
  
//   static void
//   schedule_action (automaton_interface* automaton,
// 		   void* action_entry_point,
// 		   parameter_t parameter)
//   {
//     kassert (ready_queue_ != 0);
//     kassert (automaton != 0);
//     automaton_context* ptr = static_cast<automaton_context*> (automaton->get_scheduler_context ());
//     kassert (ptr != 0);
    
//     ptr->action_entry_point = action_entry_point;
//     ptr->parameter = parameter;
    
//     if (ptr->status == NOT_SCHEDULED) {
//       ptr->status = SCHEDULED;
//       ready_queue_->push_back (ptr);
//     }
//   }
  
//   static void
//   finish_action (int output_status,
// 		 value_t output_value)
//   {
//     switch (exec_context.action_type) {
//     case INPUT:
//       /* Move to the next input. */
//       ++exec_context.input_action_pos;
//       if (exec_context.input_action_pos != exec_context.input_actions->end ()) {
// 	/* Load the execution context. */
// 	exec_context.current_automaton = exec_context.input_action_pos->automaton;
// 	exec_context.current_action_entry_point = exec_context.input_action_pos->action_entry_point;
// 	exec_context.current_parameter = exec_context.input_action_pos->parameter;
// 	/* Execute.  (This does not return). */
// 	exec_context.current_automaton->execute (exec_context.switch_stack, exec_context.switch_stack_size, exec_context.current_action_entry_point, exec_context.current_parameter, exec_context.output_value);
//       }
//       break;
//     case OUTPUT:
//       if (output_status) {
// 	/* Get the inputs that are composed with the output. */
// 	exec_context.input_actions = binding_manager<AllocatorTag, Allocator>::get_bound_inputs (exec_context.current_automaton, exec_context.current_action_entry_point, exec_context.current_parameter);
// 	if (exec_context.input_actions != 0) {
// 	  /* The output executed and there are inputs. */
// 	  exec_context.input_action_pos = exec_context.input_actions->begin ();
// 	  exec_context.output_value = output_value;
// 	  /* Load the execution context. */
// 	  exec_context.action_type = INPUT;
// 	  exec_context.current_automaton = exec_context.input_action_pos->automaton;
// 	  exec_context.current_action_entry_point = exec_context.input_action_pos->action_entry_point;
// 	  exec_context.current_parameter = exec_context.input_action_pos->parameter;
// 	  /* Execute.  (This does not return). */
// 	  exec_context.current_automaton->execute (exec_context.switch_stack, exec_context.switch_stack_size, exec_context.current_action_entry_point, exec_context.current_parameter, exec_context.output_value);
// 	}
//       }
//       break;
//     case INTERNAL:
//     case NO_ACTION:
//       break;
//     }
    
//     switch_to_next_action ();
//   }
  
// };





    // Allocate and set the switch stack for the scheduler.
    // void* switch_stack = new (AllocatorTag ()) char[SWITCH_STACK_SIZE];
    // kassert (switch_stack != 0);
    // scheduler<AllocatorTag, Allocator>::set_switch_stack (logical_address (switch_stack), SWITCH_STACK_SIZE);


#endif /* __scheduler_hpp__ */
