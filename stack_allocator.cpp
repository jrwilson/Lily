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
#include "system_allocator.hpp"

const size_t stack_allocator::MAX_REGION_SIZE = 0x07FFF000;

stack_allocator::stack_allocator (frame_t begin,
				  frame_t end) :
  begin_ (begin),
  end_ (end),
  free_head_ (0)
{
  kassert (begin < end);
  
  const frame_entry_t size = end - begin;
  entry_  = new (system_alloc ()) frame_entry_t[size];
  for (frame_entry_t k = 0; k < size; ++k) {
    entry_[k] = k + 1;
  }
  entry_[size - 1] = STACK_ALLOCATOR_EOL;
}

frame_t
stack_allocator::begin () const
{
  return begin_;
}

frame_t
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
stack_allocator::mark_as_used (frame_t frame)
{
  kassert (frame >= begin_ && frame < end_);
  
  frame_entry_t frame_idx = frame - begin_;
  // Is the frame free?
  if (entry_[frame_idx] >= 0 && entry_[frame_idx] != STACK_ALLOCATOR_EOL) {
    /* Find the one that points to it. */
    if (free_head_ != frame_idx) {
      int idx;
      for (idx = free_head_; idx != STACK_ALLOCATOR_EOL && entry_[idx] != frame_idx; idx = entry_[idx]) ;;
      kassert (idx != STACK_ALLOCATOR_EOL && entry_[idx] == frame_idx);
      /* Update the pointers. */
      entry_[idx] = entry_[frame_idx];
    }
    else {
      free_head_ = entry_[frame_idx];
    }
    entry_[frame_idx] = -1;
  }
}

frame_t
stack_allocator::alloc ()
{
  kassert (free_head_ != STACK_ALLOCATOR_EOL);
  
  frame_entry_t idx = free_head_;
  free_head_ = entry_[idx];
  entry_[idx] = -1;
  kputs (__func__); kputs (" "); kputx32 (begin_ + idx); kputs ("\n");
  return begin_ + idx;
}

size_t
stack_allocator::incref (frame_t frame)
{
  kassert (frame >= begin_ && frame < end_);
  frame_entry_t idx = frame - begin_;
  /* Frame is allocated. */
  kassert (entry_[idx] < 0);
  /* "Increment" the reference count. */
  return -(--entry_[idx]);
}

size_t
stack_allocator::decref (frame_t frame)
{
  kassert (frame >= begin_ && frame < end_);
  frame_entry_t idx = frame - begin_;
  /* Frame is allocated. */
  kassert (entry_[idx] < 0);
  /* "Decrement" the reference count. */
  size_t retval = -(++entry_[idx]);
  /* Free the frame. */
  if (entry_[idx] == 0) {
    entry_[idx] = free_head_;
    free_head_ = idx;
  }
  return retval;
}
