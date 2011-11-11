#ifndef __automaton_hpp__
#define __automaton_hpp__

/*
  File
  ----
  automaton.hpp
  
  Description
  -----------
  The automaton data structure.

  Authors:
  Justin R. Wilson
*/

#include "vm_manager.hpp"
#include "hash_map.hpp"
#include "syscall_def.hpp"
#include "vm_area.hpp"
#include "automaton_interface.hpp"

class automaton : public automaton_interface {
private:
  /* Allocator for data structures. */
  list_allocator_t* list_allocator_;
  /* Segments including privilege. */
  uint32_t code_segment_;
  uint32_t stack_segment_;
  /* Table of action descriptors for guiding execution, checking bindings, etc. */
  hash_map_t* actions_;
  /* The scheduler uses this object. */
  scheduler_context_t* scheduler_context_;
  /* Physical address of the page directory. */
  size_t page_directory_;
  /* Stack pointer (constant). */
  void* stack_pointer_;
  /* Memory map. */
  vm_area_t* memory_map_begin_;
  vm_area_t* memory_map_end_;
  /* Can map between [floor, ceiling). */
  void* memory_floor_;
  uint8_t* memory_ceiling_;
  /* Default privilege for new VM_AREA_DATA. */
  page_privilege_t page_privilege_;

  void
  merge (vm_area_t* area);

public:
  automaton (list_allocator_t* list_allocator,
	     privilege_t privilege,
	     size_t page_directory,
	     void* stack_pointer,
	     void* memory_ceiling,
	     page_privilege_t page_privilege);

  inline scheduler_context_t* get_scheduler_context (void) const {
    return scheduler_context_;
  }

  inline void* get_stack_pointer (void) const {
    return stack_pointer_;
  }
  
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

#endif /* __automaton_hpp__ */
