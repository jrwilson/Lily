#ifndef __automaton_interface_hpp__
#define __automaton_interface_hpp__

/*
  File
  ----
  automaton_interface.hpp
  
  Description
  -----------
  The interface for automata.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"
#include "action_type.hpp"

struct vm_area_base;

class automaton_interface {
public:
  virtual void*
  get_scheduler_context (void) const = 0;

  virtual logical_address
  get_stack_pointer (void) const = 0;
  
  virtual bool
  insert_vm_area (const vm_area_base& area) __attribute__((warn_unused_result)) = 0;
  
  virtual logical_address
  alloc (size_t size) __attribute__((warn_unused_result)) = 0;
  
  virtual logical_address
  reserve (size_t size) __attribute__((warn_unused_result)) = 0;
  
  virtual void
  unreserve (logical_address address) = 0;

  virtual void
  page_fault (logical_address address,
	      uint32_t error) = 0;
  
  virtual void
  add_action (void* action_entry_point,
	      action_type_t action_type) = 0;
  
  virtual action_type_t
  get_action_type (void* action_entry_point) = 0;
  
  virtual void
  execute (logical_address switch_stack,
	   size_t switch_stack_size,
	   void* action_entry_point,
	   parameter_t parameter,
	   value_t input_value) = 0;

};

#endif /* __automaton_interface_hpp__ */
