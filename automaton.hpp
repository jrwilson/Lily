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

/* Alignment of stack for switch. */
#define STACK_ALIGN 16

template<class AllocatorTag, template <typename> class Allocator>
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

  /* Segments including privilege. */
  uint32_t code_segment_;
  uint32_t stack_segment_;
  /* Table of action descriptors for guiding execution, checking bindings, etc. */
  typedef std::unordered_map<void*, action_type_t, std::hash<void*>, std::equal_to<void*>, Allocator<std::pair<void* const, action_type_t> > > action_map_type;
  action_map_type action_map_;
  /* The scheduler uses this object. */
  void* scheduler_context_;
  physical_address page_directory_;
  /* Stack pointer (constant). */
  logical_address stack_pointer_;
  /* Memory map. Consider using a set/map if insert/remove becomes too expensive. */
  typedef std::vector<vm_area_base*, Allocator<vm_area_base*> > memory_map_type;
  memory_map_type memory_map_;
  /* Default privilege for new VM_AREA_DATA. */
  page_privilege_t page_privilege_;

  typename memory_map_type::iterator
  find_by_address (const vm_area_base* area)
  {
    // Find the location to insert.
    typename memory_map_type::iterator pos = std::upper_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
    kassert (pos != memory_map_.begin ());
    return --pos;
  }

  typename memory_map_type::iterator
  find_by_size (size_t size)
  {
    typename memory_map_type::iterator pos;
    for (pos = memory_map_.begin ();
	 pos != memory_map_.end ();
	 ++pos) {
      if ((*pos)->type () == VM_AREA_FREE && size <= (*pos)->size ()) {
	break;
      }
    }
    
    return pos;
  }

  typename memory_map_type::iterator
  merge (typename memory_map_type::iterator pos)
  {
    if (pos != memory_map_.end ()) {
      typename memory_map_type::iterator next (pos);
      ++next;
      if (next != memory_map_.end ()) {
	vm_area_base* temp = *next;
	if ((*pos)->merge (*temp)) {
	  memory_map_.erase (next);
	  destroy (temp, AllocatorTag ());
	}
      }
    }
    
    return pos;
  }

  void
  insert_into_free_area (typename memory_map_type::iterator pos,
			 vm_area_base* area)
  {
    kassert ((*pos)->type () == VM_AREA_FREE);
    kassert (area->size () < (*pos)->size ());
    
    logical_address left_begin = (*pos)->begin ();
    logical_address left_end = area->begin ();
    
    logical_address right_begin = area->end ();
    logical_address right_end = (*pos)->end ();
    
    // Take out the old entry.
    destroy (*pos, AllocatorTag ());
    pos = memory_map_.erase (pos);
    
    // Split the area.
    if (right_begin < right_end) {
      pos = merge (memory_map_.insert (pos, new (AllocatorTag ()) vm_free_area<AllocatorTag> (right_begin, right_end)));
    }
    pos = merge (memory_map_.insert (pos, area));
    if (left_begin < left_end) {
      pos = merge (memory_map_.insert (pos, new (AllocatorTag ()) vm_free_area<AllocatorTag> (left_begin, left_end)));
    }
    
    if (pos != memory_map_.begin ()) {
      merge (--pos);
    }
    
  }

public:
  automaton (privilege_t privilege,
	     physical_address page_directory,
	     logical_address stack_pointer,
	     logical_address memory_ceiling,
	     page_privilege_t page_privilege) :
    scheduler_context_ (scheduler<AllocatorTag, Allocator>::allocate_context (this)),
    page_directory_ (page_directory),
    stack_pointer_ (stack_pointer),
    page_privilege_ (page_privilege)
  {
    switch (privilege) {
    case RING0:
      code_segment_ = KERNEL_CODE_SELECTOR | RING0;
      stack_segment_ = KERNEL_DATA_SELECTOR | RING0;
      break;
    case RING1:
    case RING2:
      /* These rings are not supported. */
      kassert (0);
      break;
    case RING3:
      /* Not supported yet. */
      kassert (0);
      break;
    }
    
    memory_ceiling.align_up (PAGE_SIZE);
    
    memory_map_.push_back (new (AllocatorTag ()) vm_free_area<AllocatorTag> (logical_address (0), memory_ceiling));
    memory_map_.push_back (new (AllocatorTag ()) vm_reserved_area<AllocatorTag> (memory_ceiling, logical_address (0)));
  }

  inline void* get_scheduler_context (void) const {
    return scheduler_context_;
  }

  inline logical_address get_stack_pointer (void) const {
    return stack_pointer_;
  }
  
  bool
  insert_vm_area (const vm_area_base& area) __attribute__((warn_unused_result))
  {
    kassert (area.end () <= memory_map_.back ()->begin ());
    
    // Find the location to insert.
    typename memory_map_type::iterator pos = find_by_address (&area);
    
    if ((*pos)->type () == VM_AREA_FREE && area.size () < (*pos)->size ()) {
      insert_into_free_area (pos, area.clone ());
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
    
    typename memory_map_type::iterator pos = find_by_size (size);
    if (pos != memory_map_.end ()) {
      logical_address retval ((*pos)->begin ());
      insert_into_free_area (pos, new (AllocatorTag ()) vm_data_area<AllocatorTag> (retval, retval + size, page_privilege_));
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
    
    typename memory_map_type::iterator pos = find_by_size (size);
    
    if (pos != memory_map_.end ()) {
      logical_address retval ((*pos)->begin ());
      insert_into_free_area (pos, new (AllocatorTag ()) vm_reserved_area<AllocatorTag> ((*pos)->begin (), (*pos)->begin () + size));
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
    
    vm_reserved_area<AllocatorTag> k (address, address + PAGE_SIZE);
    
    typename memory_map_type::iterator pos = find_by_address (&k);
    kassert (pos != memory_map_.end ());
    kassert ((*pos)->begin () == address);
    kassert ((*pos)->type () == VM_AREA_RESERVED);
    
    vm_free_area<AllocatorTag>* f = new (AllocatorTag ()) vm_free_area<AllocatorTag> ((*pos)->begin (), (*pos)->end ());
    
    destroy (*pos, AllocatorTag ());
    pos = memory_map_.insert (memory_map_.erase (pos), f);
    
    merge (pos);
    if (pos != memory_map_.begin ()) {
      merge (--pos);
    }
  }
  
  void
  page_fault (logical_address address,
	      uint32_t error)
  {
    vm_reserved_area<AllocatorTag> k (address, address + 1);
    
    typename memory_map_type::const_iterator pos = find_by_address (&k);
    
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
    typename action_map_type::const_iterator pos = action_map_.find (action_entry_point);
    if (pos != action_map_.end ()) {
      return pos->second;
    }
    else {
      return NO_ACTION;
    }
  }
  
  void
  execute (logical_address switch_stack,
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
    new_stack_begin = (switch_stack + (switch_stack_size - stack_size)).align_down (STACK_ALIGN);
    /* Copy. */
    for (idx = 0; idx < stack_size; ++idx) {
      new_stack_begin[idx] = stack_begin[idx];
    }
    
    /* Update the base and stack pointers. */
    asm volatile ("add %0, %%esp\n"
		  "add %0, %%ebp\n" :: "r"(new_stack_begin - stack_begin) : "%esp", "memory");
    
    /* Switch page directories. */
    vm_manager_switch_to_directory (page_directory_);
    
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
