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

void
frame_manager::mark_as_used (const frame& frame)
{
  stack_allocators_type::iterator pos = find_allocator (frame);

  if (pos != stack_allocators_.end ()) {
    (*pos)->mark_as_used (frame);
  }
}

// frame
// frame_manager::alloc ()
// {
//   stack_allocator_t* ptr;
//   /* Find an allocator with a free frame. */
//   for (ptr = stack_allocators_; ptr != 0 && ptr->free_head == STACK_ALLOCATOR_EOL; ptr = ptr->next) ;;

//   /* Out of frames. */
//   kassert (ptr != 0);

//   return stack_allocator_alloc (ptr);
// }

// void
// frame_manager::incref (const frame& frame)
// {
//   stack_allocator_t* ptr = find_allocator (frame);

//   /* No allocator for frame. */
//   kassert (ptr != 0);

//   stack_allocator_incref (ptr, frame);
// }

// void
// frame_manager::decref (const frame& frame)
// {
//   stack_allocator_t* ptr = find_allocator (frame);

//   /* No allocator for frame. */
//   kassert (ptr != 0);

//   stack_allocator_decref (ptr, frame);
// }
