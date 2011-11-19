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
#include <unordered_map>

class automaton : public automaton_interface {
private:
  /* Allocator for data structures. */
  list_allocator& list_allocator_;
  /* Segments including privilege. */
  uint32_t code_segment_;
  uint32_t stack_segment_;
  /* Table of action descriptors for guiding execution, checking bindings, etc. */
  typedef std::unordered_map<void*, action_type_t> action_map_type;
  action_map_type action_map_;
  /* The scheduler uses this object. */
  scheduler_context_t* scheduler_context_;
  physical_address page_directory_;
  /* Stack pointer (constant). */
  logical_address stack_pointer_;
  /* Memory map. */
  vm_area* memory_map_begin_;
  vm_area* memory_map_end_;
  /* Can map between [floor, ceiling). */
  void* memory_floor_;
  logical_address memory_ceiling_;
  /* Default privilege for new VM_AREA_DATA. */
  page_privilege_t page_privilege_;

  void
  merge (vm_area* area);

public:
  automaton (list_allocator& list_allocator,
	     privilege_t privilege,
	     physical_address page_directory,
	     logical_address stack_pointer,
	     logical_address memory_ceiling,
	     page_privilege_t page_privilege);

  inline scheduler_context_t* get_scheduler_context (void) const {
    return scheduler_context_;
  }

  inline logical_address get_stack_pointer (void) const {
    return stack_pointer_;
  }
  
  int
  insert_vm_area (const vm_area* area) __attribute__((warn_unused_result));
  
  logical_address
  alloc (size_t size,
	 syserror_t* error) __attribute__((warn_unused_result));
  
  logical_address
  reserve (size_t size);
  
  void
  unreserve (logical_address);

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

#endif /* __automaton_hpp__ */
