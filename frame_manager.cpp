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

size_t frame_manager::stack_allocator_size_;
stack_allocator** frame_manager::stack_allocator_;

void
frame_manager::mark_as_used (const frame& frame)
{
  stack_allocator** pos = find_allocator (frame);

  if (pos != stack_allocator_ + stack_allocator_size_) {
    (*pos)->mark_as_used (frame);
  }
}

frame
frame_manager::alloc ()
{
  /* Find an allocator with a free frame. */
  stack_allocator** pos = std::find_if (stack_allocator_, stack_allocator_ + stack_allocator_size_, stack_allocator_not_full ());
  
  /* Out of frames. */
  kassert (pos != stack_allocator_ + stack_allocator_size_);
  
  return (*pos)->alloc ();
}

void
frame_manager::incref (const frame& frame)
{
  stack_allocator** pos = find_allocator (frame);

  /* No allocator for frame. */
  kassert (pos != stack_allocator_ + stack_allocator_size_);

  (*pos)->incref (frame);
}

// void
// frame_manager::decref (const frame& frame)
// {
//   stack_allocator_t* ptr = find_allocator (frame);

//   /* No allocator for frame. */
//   kassert (ptr != 0);

//   stack_allocator_decref (ptr, frame);
// }
