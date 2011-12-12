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
#include <utility>
#include "vm_def.hpp"
#include <stddef.h>

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

class system_alloc;

class stack_allocator {
private:
  typedef int16_t frame_entry_t;
  static const frame_entry_t STACK_ALLOCATOR_EOL = -32768;

  frame_t begin_;
  frame_t end_;
  frame_entry_t free_head_;
  frame_entry_t* entry_;

public:
  static const size_t MAX_REGION_SIZE;

  stack_allocator (frame_t begin,
		   frame_t end);

  frame_t
  begin () const;

  frame_t
  end () const;

  bool
  full () const;

  void
  mark_as_used (frame_t frame);

  frame_t
  alloc ();

  size_t
  incref (frame_t frame);

  size_t
  decref (frame_t frame);
};

#endif /* __stack_allocator_hpp__ */
