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

#include "multiboot.h"

void
frame_manager_initialize (const multiboot_info_t* mbd);

/* Return the physical and logical locations of the frame manager in memory. */
unsigned int
frame_manager_physical_begin (void);

unsigned int
frame_manager_physical_end (void);

void*
frame_manager_logical_begin (void);

void*
frame_manager_logical_end (void);

/* This function allows a frame to be marked as used when initializing virtual memory. */
void
frame_manager_mark_as_used (unsigned int frame);

/* Allocate a frame. */
unsigned int
frame_manager_allocate () __attribute__((warn_unused_result));

/* Increment the reference count for a frame. */
void
frame_mananger_incref (unsigned int frame);

/* Decrement the reference count for a frame. */
void
frame_manager_decref (unsigned int frame);

#endif /* __frame_manager_h__ */
