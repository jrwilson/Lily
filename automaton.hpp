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
#include "syscall_def.hpp"
#include "vm_area.hpp"
#include "automaton_interface.hpp"
#include <unordered_map>
#include <vector>
#include "scheduler.hpp"
#include "global_descriptor_table.hpp"
#include "list_allocator.hpp"

/* Alignment of stack for switch. */
#define STACK_ALIGN 16

class automaton : public automaton_interface {
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
  /* The scheduler uses this object. */
  void* scheduler_context_;
  physical_address page_directory_;
  /* Stack pointer (constant). */
  logical_address stack_pointer_;
  /* Memory map. Consider using a set/map if insert/remove becomes too expensive. */
  typedef std::vector<vm_area_base*, list_allocator<vm_area_base*> > memory_map_type;
  memory_map_type memory_map_;
  /* Default privilege for new VM_AREA_DATA. */
  paging_constants::page_privilege_t page_privilege_;

  memory_map_type::iterator
  find_by_address (const vm_area_base* area)
  {
    // Find the location to insert.
    memory_map_type::iterator pos = std::upper_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
    kassert (pos != memory_map_.begin ());
    return --pos;
  }

  memory_map_type::iterator
  find_by_size (size_t size)
  {
    memory_map_type::iterator pos;
    for (pos = memory_map_.begin ();
	 pos != memory_map_.end ();
	 ++pos) {
      if ((*pos)->type () == VM_AREA_FREE && size <= (*pos)->size ()) {
	break;
      }
    }
    
    return pos;
  }

  memory_map_type::iterator
  merge (memory_map_type::iterator pos)
  {
    if (pos != memory_map_.end ()) {
      memory_map_type::iterator next (pos);
      ++next;
      if (next != memory_map_.end ()) {
	vm_area_base* temp = *next;
	if ((*pos)->merge (*temp)) {
	  memory_map_.erase (next);
	  destroy (temp, alloc_);
	}
      }
    }
    
    return pos;
  }

  void
  insert_into_free_area (memory_map_type::iterator pos,
			 vm_area_base* area)
  {
    kassert ((*pos)->type () == VM_AREA_FREE);
    kassert (area->size () < (*pos)->size ());
    
    logical_address left_begin = (*pos)->begin ();
    logical_address left_end = area->begin ();
    
    logical_address right_begin = area->end ();
    logical_address right_end = (*pos)->end ();
    
    // Take out the old entry.
    destroy (*pos, alloc_);
    pos = memory_map_.erase (pos);
    
    // Split the area.
    if (right_begin < right_end) {
      pos = merge (memory_map_.insert (pos, new (alloc_) vm_free_area (right_begin, right_end)));
    }
    pos = merge (memory_map_.insert (pos, area));
    if (left_begin < left_end) {
      pos = merge (memory_map_.insert (pos, new (alloc_) vm_free_area (left_begin, left_end)));
    }
    
    if (pos != memory_map_.begin ()) {
      merge (--pos);
    }
    
  }

public:
  automaton (list_alloc& a,
	     descriptor_constants::privilege_t privilege,
	     physical_address page_directory,
	     logical_address stack_pointer,
	     logical_address memory_ceiling,
	     paging_constants::page_privilege_t page_privilege) :
    alloc_ (a),
    //scheduler_context_ (scheduler<AllocatorTag, Allocator>::allocate_context (this)),
    action_map_ (3, action_map_type::hasher (), action_map_type::key_equal (), action_map_type::allocator_type (alloc_)),
    page_directory_ (page_directory),
    stack_pointer_ (stack_pointer),
    memory_map_ (memory_map_type::allocator_type (alloc_)),
    page_privilege_ (page_privilege)
  {
    switch (privilege) {
    case descriptor_constants::RING0:
      code_segment_ = KERNEL_CODE_SELECTOR | descriptor_constants::RING0;
      stack_segment_ = KERNEL_DATA_SELECTOR | descriptor_constants::RING0;
      break;
    case descriptor_constants::RING1:
    case descriptor_constants::RING2:
      /* These rings are not supported. */
      kassert (0);
      break;
    case descriptor_constants::RING3:
      /* Not supported yet. */
      kassert (0);
      break;
    }
    
    memory_ceiling <<= PAGE_SIZE;
    
    memory_map_.push_back (new (alloc_) vm_free_area (logical_address (0), memory_ceiling));
    memory_map_.push_back (new (alloc_) vm_reserved_area (memory_ceiling, logical_address (0)));
  }

  inline void* get_scheduler_context (void) const {
    return scheduler_context_;
  }

  inline logical_address get_stack_pointer (void) const {
    return stack_pointer_;
  }
  
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
  alloc (size_t size) __attribute__((warn_unused_result))
  {
    kassert (size > 0);
    
    memory_map_type::iterator pos = find_by_size (size);
    if (pos != memory_map_.end ()) {
      logical_address retval ((*pos)->begin ());
      insert_into_free_area (pos, new (alloc_) vm_data_area (retval, retval + size, page_privilege_));
      return retval;
    }
    else {
      return logical_address ();
    }
  }

  logical_address
  reserve (size_t size)
  {
    kassert (size > 0);
    
    memory_map_type::iterator pos = find_by_size (size);
    
    if (pos != memory_map_.end ()) {
      logical_address retval ((*pos)->begin ());
      insert_into_free_area (pos, new (alloc_) vm_reserved_area ((*pos)->begin (), (*pos)->begin () + size));
      return retval;
    }
    else {
      return logical_address ();
    }
  }
  
  void
  unreserve (logical_address address)
  {
    kassert (address != logical_address ());
    kassert (address.is_aligned (PAGE_SIZE));
    
    vm_reserved_area k (address, address + PAGE_SIZE);
    
    memory_map_type::iterator pos = find_by_address (&k);
    kassert (pos != memory_map_.end ());
    kassert ((*pos)->begin () == address);
    kassert ((*pos)->type () == VM_AREA_RESERVED);
    
    vm_free_area* f = new (alloc_) vm_free_area ((*pos)->begin (), (*pos)->end ());
    
    destroy (*pos, alloc_);
    pos = memory_map_.insert (memory_map_.erase (pos), f);
    
    merge (pos);
    if (pos != memory_map_.begin ()) {
      merge (--pos);
    }
  }
  
  void
  page_fault (frame_manager&,
	      vm_manager&,
	      logical_address address,
	      uint32_t error)
  {
    vm_reserved_area k (address, address + 1);
    
    memory_map_type::const_iterator pos = find_by_address (&k);
    
    if (pos != memory_map_.end ()) {
      (*pos)->page_fault (address, error);
    }
    else {
      /* TODO:  Not in memory map. */
      kassert (0);
    }
  }
  
  void
  add_action (void* action_entry_point,
	      action_type_t action_type)
  {
    kassert (action_map_.insert (std::make_pair (action_entry_point, action_type)).second);
  }
  
  action_type_t
  get_action_type (void* action_entry_point)
  {
    action_map_type::const_iterator pos = action_map_.find (action_entry_point);
    if (pos != action_map_.end ()) {
      return pos->second;
    }
    else {
      return NO_ACTION;
    }
  }
  
  void
  execute (vm_manager& vm,
	   logical_address switch_stack,
	   size_t switch_stack_size,
	   void* action_entry_point,
	   parameter_t parameter,
	   value_t input_value)
  {
    logical_address stack_begin;
    logical_address stack_end;
    logical_address new_stack_begin;
    size_t stack_size;
    size_t idx;
    
    /* Move the stack into an area mapped in all address spaces so that switching page directories doesn't cause a triple fault. */
    
    /* Determine the beginning of the stack. */
    asm volatile ("mov %%esp, %0\n" : "=m"(stack_begin));
    /* Determine the end of the stack. */
    stack_end = logical_address (&input_value);
    stack_end += sizeof (size_t);
    /* Compute the beginning of the new stack. */
    stack_size = stack_end - stack_begin;
    /* Use a bigger switch stack. */
    kassert (stack_size < switch_stack_size);
    new_stack_begin = (switch_stack + (switch_stack_size - stack_size)) >> STACK_ALIGN;
    /* Copy. */
    for (idx = 0; idx < stack_size; ++idx) {
      new_stack_begin[idx] = stack_begin[idx];
    }
    
    /* Update the base and stack pointers. */
    asm volatile ("add %0, %%esp\n"
		  "add %0, %%ebp\n" :: "r"(new_stack_begin - stack_begin) : "%esp", "memory");
    
    /* Switch page directories. */
    vm.switch_to_directory (page_directory_);
    
    /* Load the new stack segment.
       Load the new stack pointer.
       Enable interrupts on return.
       Load the new code segment.
       Load the new instruction pointer.
       Load the parameter.
       Load the value for input actions.
    */
    asm volatile ("mov %0, %%eax\n"
		  "mov %%ax, %%ss\n"
		  "mov %1, %%eax\n"
		  "mov %%eax, %%esp\n"
		  "pushf\n"
		  "pop %%eax\n"
		  "or $0x200, %%eax\n"
		  "push %%eax\n"
		  "pushl %2\n"
		  "pushl %3\n"
		  "movl %4, %%ecx\n"
		  "movl %5, %%edx\n"
		  "iret\n" :: "m"(stack_segment_), "m"(stack_pointer_), "m"(code_segment_), "m"(action_entry_point), "m"(parameter), "m"(input_value));
  }
  
};

#endif /* __automaton_hpp__ */
