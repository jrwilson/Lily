/*
  File
  ----
  frame_manager.c
  
  Description
  -----------
  The frame manager manages physical memory.

  Authors:
  Justin R. Wilson
*/

#include "frame_manager.hpp"
#include "vm_def.hpp"

void
frame_manager::add (physical_address_t begin,
		    physical_address_t end)
{
  kassert (begin >= USABLE_MEMORY_BEGIN);
  kassert (end <= USABLE_MEMORY_END);
  kassert (is_aligned (begin, PAGE_SIZE));
  kassert (is_aligned (end, PAGE_SIZE));
  kassert (begin < end);
  
  size_t size = end - begin;
  while (size != 0) {
    size_t sz = min (size_t (stack_allocator::MAX_REGION_SIZE), size);
    allocator_list_.push_back (new stack_allocator (physical_address_to_frame (begin), physical_address_to_frame (begin + sz)));
    size -= sz;
    begin += sz;
  }
}

void
frame_manager::mark_as_used (frame_t frame)
{
  allocator_list_type::iterator pos = find_allocator (frame);

  if (pos != allocator_list_.end ()) {
    (*pos)->mark_as_used (frame);
  }
}

frame_manager::allocator_list_type frame_manager::allocator_list_;
