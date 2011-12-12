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
#include "kassert.hpp"
#include <algorithm>
#include <utility>

using namespace std::rel_ops;

frame_manager::allocator_list_type frame_manager::allocator_list_;

void
frame_manager::mark_as_used (frame_t frame)
{
  allocator_list_type::iterator pos = find_allocator (frame);

  if (pos != allocator_list_.end ()) {
    (*pos)->mark_as_used (frame);
  }
}

frame_t
frame_manager::alloc ()
{
  /* Find an allocator with a free frame. */
  allocator_list_type::iterator pos = std::find_if (allocator_list_.begin (), allocator_list_.end (), stack_allocator_not_full ());
  
  /* Out of frames. */
  kassert (pos != allocator_list_.end ());
  
  return (*pos)->alloc ();
}

size_t
frame_manager::incref (frame_t frame)
{
  allocator_list_type::iterator pos = find_allocator (frame);

  /* No allocator for frame. */
  kassert (pos != allocator_list_.end ());

  return (*pos)->incref (frame);
}

size_t
frame_manager::decref (frame_t frame)
{
  allocator_list_type::iterator pos = find_allocator (frame);

  /* No allocator for frame. */
  kassert (pos != allocator_list_.end ());

  return (*pos)->decref (frame);
}
