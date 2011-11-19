#ifndef __boot_automaton_hpp__
#define __boot_automaton_hpp__

/*
  File
  ----
  boot_automaton.hpp
  
  Description
  -----------
  Special automaton for booting.

  Authors:
  Justin R. Wilson
*/

#include "automaton_interface.hpp"

class boot_automaton : public automaton_interface {
private:
  vm_area data_;
public:
  boot_automaton (logical_address begin,
		  logical_address end);
  
  const vm_area*
  get_data_area (void) const;

  scheduler_context_t*
  get_scheduler_context (void) const;

  logical_address
  get_stack_pointer (void) const;
  
  int
  insert_vm_area (const vm_area* area) __attribute__((warn_unused_result));
  
  logical_address
  alloc (size_t size,
	 syserror_t* error) __attribute__((warn_unused_result));
  
  logical_address
  reserve (size_t size);
  
  void
  unreserve (logical_address address);
  
  void
  page_fault (logical_address address,
	      uint32_t error);

  void
  add_action (void* action_entry_point,
	      action_type_t action_type);
  
  action_type_t
  get_action_type (void* action_entry_point);
  
  void
  execute (logical_address switch_stack,
	   size_t switch_stack_size,
	   void* action_entry_point,
	   parameter_t parameter,
	   value_t input_value);
};

#endif /* __boot_automaton_hpp__ */
