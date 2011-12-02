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

#include "automaton.hpp"
#include "global_descriptor_table.hpp"

automaton::memory_map_type::iterator
automaton::find_by_address (const vm_area_base* area)
{
  // Find the location to insert.
  memory_map_type::iterator pos = std::upper_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
  kassert (pos != memory_map_.begin ());
  return --pos;
}

automaton::memory_map_type::iterator
automaton::find_by_size (size_t size)
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

automaton::memory_map_type::iterator
automaton::merge (memory_map_type::iterator pos)
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
automaton::insert_into_free_area (memory_map_type::iterator pos,
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

automaton::automaton (list_alloc& a,
		      descriptor_constants::privilege_t privilege,
		      physical_address page_directory,
		      logical_address stack_pointer,
		      logical_address memory_ceiling,
		      paging_constants::page_privilege_t page_privilege) :
  alloc_ (a),
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

logical_address
automaton::get_stack_pointer (void) const {
  return stack_pointer_;
}

logical_address
automaton::alloc (size_t size)
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
automaton::reserve (size_t size)
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
automaton::unreserve (logical_address address)
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
automaton::page_fault (logical_address address,
		       uint32_t error)
{
  vm_reserved_area k (address, address + 1);
  memory_map_type::const_iterator pos = find_by_address (&k);
  kassert (pos != memory_map_.end ());
  (*pos)->page_fault (address, error);
}
  
void
automaton::add_action (void* action_entry_point,
		       action_type_t action_type)
{
  kassert (action_map_.insert (std::make_pair (action_entry_point, action_type)).second);
}
  
action_type_t
automaton::get_action_type (void* action_entry_point)
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
automaton::execute (logical_address switch_stack,
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
  vm_manager::switch_to_directory (page_directory_);
  
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

