#ifndef __frame_manager_h__
#define __frame_manager_h__

/*
  File
  ----
  frame_manager.h
  
  Description
  -----------
  The frame manager manages page-sized units of physical memory called frames.
  Frames are reference counted so that frames can be shared.

  Authors:
  Justin R. Wilson
*/

#include "placement_allocator.hpp"
#include "types.hpp"

typedef uint32_t frame_t;

void
frame_manager_initialize (placement_allocator_t*);

void
frame_manager_add (placement_allocator_t*,
		   uint64_t first,
		   uint64_t last);

/* This function allows a frame to be marked as used when initializing virtual memory. */
void
frame_manager_mark_as_used (frame_t frame);

/* Allocate a frame. */
frame_t
frame_manager_alloc () __attribute__((warn_unused_result));

/* Increment the reference count for a frame. */
void
frame_manager_incref (frame_t frame);

/* Decrement the reference count for a frame. */
void
frame_manager_decref (frame_t frame);

#endif /* __frame_manager_h__ */
