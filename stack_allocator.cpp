/*
  File
  ----
  stack_allocator.hpp
  
  Description
  -----------
  Allocate frames in a region of memory using a stack.

  Authors:
  Justin R. Wilson
*/

#include "stack_allocator.hpp"

const size_t stack_allocator::MAX_REGION_SIZE = 0x07FFF000;

stack_allocator::stack_allocator (size_t begin,
				  size_t end,
				  placement_alloc& alloc) :
  begin_ (begin),
  end_ (end),
  free_head_ (0)
{
  kassert (begin < end);
  
  const frame_entry_t size = end - begin;
  entry_  = new (alloc) frame_entry_t[size];
  for (frame_entry_t k = 0; k < size; ++k) {
    entry_[k] = k + 1;
  }
  entry_[size - 1] = STACK_ALLOCATOR_EOL;
}

size_t
stack_allocator::begin () const
{
  return begin_;
}

size_t
stack_allocator::end () const
{
  return end_;
}

bool
stack_allocator::full () const
{
  return free_head_ == STACK_ALLOCATOR_EOL;
}

void
stack_allocator::mark_as_used (size_t frame)
{
  kassert (frame >= begin_ && frame < end_);
  
  frame_entry_t frame_idx = frame - begin_;
  /* Should be free. */
  kassert (entry_[frame_idx] >= 0 && entry_[frame_idx] != STACK_ALLOCATOR_EOL);
  /* Find the one that points to it. */
  int idx;
  for (idx = free_head_; idx != STACK_ALLOCATOR_EOL && entry_[idx] != frame_idx; idx = entry_[idx]) ;;
  kassert (idx != STACK_ALLOCATOR_EOL && entry_[idx] == frame_idx);
  /* Update the pointers. */
  entry_[idx] = entry_[frame_idx];
  entry_[frame_idx] = -1;
}

size_t
stack_allocator::alloc ()
{
  kassert (free_head_ != STACK_ALLOCATOR_EOL);
  
  frame_entry_t idx = free_head_;
  free_head_ = entry_[idx];
  entry_[idx] = -1;
  return begin_ + idx;
}

void
stack_allocator::incref (size_t frame)
{
  kassert (frame >= begin_ && frame < end_);
  frame_entry_t idx = frame - begin_;
  /* Frame is allocated. */
  kassert (entry_[idx] < 0);
  /* "Increment" the reference count. */
  --entry_[idx];
}

// static void
// stack_allocator_decref (stack_allocator_t* ptr,
// 			frame frame)
// {
//   kassert (ptr != 0);
//   kassert (frame >= ptr->begin && frame < ptr->end);
//   frame_entry_t idx = frame - ptr->begin;
//   /* Frame is allocated. */
//   kassert (ptr->entry[idx] < 0);
//   /* "Decrement" the reference count. */
//   ++ptr->entry[idx];
//   /* Free the frame. */
//   if (ptr->entry[idx]) {
//     ptr->entry[idx] = ptr->free_head;
//     ptr->free_head = idx;
//   }
// }
