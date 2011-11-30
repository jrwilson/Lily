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
#include "frame.hpp"
#include "placement_allocator.hpp"
#include <utility>

using namespace std::rel_ops;

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

class stack_allocator {
private:
  typedef int16_t frame_entry_t;
  static const frame_entry_t STACK_ALLOCATOR_EOL = -32768;

  frame begin_;
  frame end_;
  frame_entry_t free_head_;
  frame_entry_t* entry_;

public:
  static const size_t MAX_REGION_SIZE;

  stack_allocator (frame begin,
		   frame end,
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

  inline frame
  begin () const
  {
    return begin_;
  }

  inline frame
  end () const
  {
    return end_;
  }

  void
  mark_as_used (frame frame)
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

// static frame
// stack_allocator_alloc (stack_allocator_t* ptr)
// {
//   kassert (ptr != 0);
//   kassert (ptr->free_head != STACK_ALLOCATOR_EOL);

//   frame_entry_t idx = ptr->free_head;
//   ptr->free_head = ptr->entry[idx];
//   ptr->entry[idx] = -1;
//   frame retval (ptr->begin);
//   retval += idx;
//   return retval;
// }

// static void
// stack_allocator_incref (stack_allocator_t* ptr,
// 			frame frame)
// {
//   kassert (ptr != 0);
//   kassert (frame >= ptr->begin && frame < ptr->end);
//   frame_entry_t idx = frame - ptr->begin;
//   /* Frame is allocated. */
//   kassert (ptr->entry[idx] < 0);
//   /* "Increment" the reference count. */
//   --ptr->entry[idx];
// }

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

};

#endif /* __stack_allocator_hpp__ */
