#ifndef __frame_manager_hpp__
#define __frame_manager_hpp__

/*
  File
  ----
  frame_manager.hpp
  
  Description
  -----------
  The frame manager manages page-sized units of physical memory called frames.
  Frames are reference counted so that frames can be shared.

  Authors:
  Justin R. Wilson
*/

#include "placement_allocator.hpp"
#include "types.hpp"
#include "frame.hpp"

/* Don't mess with memory below 1M or above 4G. */
const physical_address USABLE_MEMORY_BEGIN (0x00100000);
const physical_address USABLE_MEMORY_END (0xFFFFF000);

struct stack_allocator_t;

class frame_manager {
private:
  static stack_allocator_t* stack_allocators_;

  static stack_allocator_t*
  find_allocator (frame frame);

public:
  static void
  add (placement_allocator&,
       physical_address begin,
       physical_address end);
  
  /* This function allows a frame to be marked as used when initializing virtual memory. */
  static void
  mark_as_used (const frame& frame);
  
  /* Allocate a frame. */
  static frame
  alloc () __attribute__((warn_unused_result));
  
  /* Increment the reference count for a frame. */
  static void
  incref (const frame& frame);
  
  /* Decrement the reference count for a frame. */
  static void
  decref (const frame& frame);
};


#endif /* __frame_manager_hpp__ */
