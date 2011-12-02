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

#include "syscall_def.hpp"
#include "vm_area.hpp"
#include <unordered_map>
#include <vector>
#include "list_allocator.hpp"
#include "action_type.hpp"
#include "descriptor.hpp"

/* Alignment of stack for switch. */
#define STACK_ALIGN 16

class automaton {
private:
  struct compare_vm_area {
    bool
    operator () (const vm_area_base* const x,
		 const vm_area_base* const y) const
    {
      return x->begin () < y->begin ();
    }
  };

  list_alloc& alloc_;
  /* Segments including privilege. */
  uint32_t code_segment_;
  uint32_t stack_segment_;
  /* Table of action descriptors for guiding execution, checking bindings, etc. */
  typedef std::unordered_map<void*, action_type_t, std::hash<void*>, std::equal_to<void*>, list_allocator<std::pair<void* const, action_type_t> > > action_map_type;
  action_map_type action_map_;
  physical_address page_directory_;
  /* Stack pointer (constant). */
  logical_address stack_pointer_;
  /* Memory map. Consider using a set/map if insert/remove becomes too expensive. */
  typedef std::vector<vm_area_base*, list_allocator<vm_area_base*> > memory_map_type;
  memory_map_type memory_map_;
  /* Default privilege for new VM_AREA_DATA. */
  paging_constants::page_privilege_t page_privilege_;

  memory_map_type::iterator
  find_by_address (const vm_area_base* area);

  memory_map_type::iterator
  find_by_size (size_t size);

  memory_map_type::iterator
  merge (memory_map_type::iterator pos);

  void
  insert_into_free_area (memory_map_type::iterator pos,
			 vm_area_base* area);

public:
  automaton (list_alloc& a,
	     descriptor_constants::privilege_t privilege,
	     physical_address page_directory,
	     logical_address stack_pointer,
	     logical_address memory_ceiling,
	     paging_constants::page_privilege_t page_privilege);

  logical_address
  get_stack_pointer (void) const;
  
  template <class T>
  bool
  insert_vm_area (const T& area)
  {
    kassert (area.end () <= memory_map_.back ()->begin ());
    
    // Find the location to insert.
    memory_map_type::iterator pos = find_by_address (&area);
    
    if ((*pos)->type () == VM_AREA_FREE && area.size () < (*pos)->size ()) {
      insert_into_free_area (pos, new (alloc_) T (area));
      return true;
    }
    else {
      return false;
    }
  }
  
  logical_address
  alloc (size_t size) __attribute__((warn_unused_result));

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

#endif /* __automaton_hpp__ */
