#ifndef __stack_allocator_hpp__
#define __stack_allocator_hpp__

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

#include "kassert.hpp"
#include "vm_def.hpp"
#include <stddef.h>

/*
  A frame entry stores the next entry on the free list or the reference count.
  Next has the range [0, size - 1].
  STACK_ALLOCATOR_EOL marks the end of the list.
  Reference count is the additive inverse of the reference count.

  The goal is to support 4GB of memory.
  4GB / PAGE_SIZE = 1,048,576 frames.
  The two choices are to use 1 table with 31-bit indexing or multiple tables with 15-bit indexing.
  The space required to hold the table entries is:
  1,048,576 frames * 4 bytes/frame = 4MB
  1,048,576 frames * 2 bytes/frame = 2MB
  The non-entry overhead is negligible, i.e., propotional to the number of regions multiplied by sizeof (stack_allocator_t).

  With 31-bit entries, a frame can be shared 2,147,483,647 times.
  With 15-bit entries, a frame can be shared 32,767 times.

  Share a page 32,767 times seems reasonable so I will use the more space-efficient 15-bit entries.
*/

class kernel_alloc;

class stack_allocator {
public:
  static const size_t MAX_REGION_SIZE = 0x07FFF000;

  stack_allocator (frame_t begin,
		   frame_t end);

  inline frame_t
  begin () const
  {
    return begin_;
  }

  inline frame_t
  end () const
  {
    return end_;
  }
  
  inline bool
  full () const
  {
    return free_head_ == STACK_ALLOCATOR_EOL;
  }
  
  void
  mark_as_used (frame_t frame);

  inline frame_t
  alloc ()
  {
    kassert (free_head_ != STACK_ALLOCATOR_EOL);
    
    frame_entry_t idx = free_head_;
    free_head_ = entry_[idx];
    entry_[idx] = -1;
    return begin_ + idx;
  }

  inline size_t
  incref (frame_t frame,
	  size_t count)
  {
    kassert (frame >= begin_ && frame < end_);
    frame_entry_t idx = frame - begin_;
    /* Frame is allocated. */
    kassert (entry_[idx] < 0);
    /* "Increment" the reference count. */
    return -(entry_[idx] -= count);
  }

  inline size_t
  decref (frame_t frame)
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

private:
  typedef int16_t frame_entry_t;
  static const frame_entry_t STACK_ALLOCATOR_EOL = -32768;

  frame_t begin_;
  frame_t end_;
  frame_entry_t free_head_;
  frame_entry_t* entry_;
};

#endif /* __stack_allocator_hpp__ */
