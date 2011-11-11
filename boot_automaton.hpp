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
  vm_area_t data_;
public:
  boot_automaton (void* begin,
		  void* end);
  
  const vm_area_t*
  get_data_area (void) const;

  scheduler_context_t*
  get_scheduler_context (void) const;

  void*
  get_stack_pointer (void) const;
  
  int
  insert_vm_area (const vm_area_t* area) __attribute__((warn_unused_result));
  
  void*
  alloc (size_t size,
	 syserror_t* error) __attribute__((warn_unused_result));
  
  void*
  reserve (size_t size);
  
  void
  unreserve (void* ptr);
  
  void
  page_fault (void* address,
	      uint32_t error);

  void
  set_action_type (void* action_entry_point,
		   action_type_t action_type);
  
  action_type_t
  get_action_type (void* action_entry_point);
  
  void
  execute (void* switch_stack,
	   size_t switch_stack_size,
	   void* action_entry_point,
	   parameter_t parameter,
	   value_t input_value);
};

#endif /* __boot_automaton_hpp__ */
