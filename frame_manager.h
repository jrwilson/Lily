#ifndef __frame_manager_h__
#define __frame_manager_h__

/*
  File
  ----
  frame_manager.h
  
  Description
  -----------
  The frame manager manages physical memory.

  Authors:
  Justin R. Wilson
*/

#include "multiboot.h"

void
frame_manager_initialize (const multiboot_info_t* mbd);

unsigned int
frame_manager_physical_begin (void);

unsigned int
frame_manager_physical_end (void);

void*
frame_manager_logical_begin (void);

void*
frame_manager_logical_end (void);

void
frame_manager_mark_as_used (unsigned int frame);

unsigned int
frame_manager_allocate () __attribute__((warn_unused_result));

#endif /* __frame_manager_h__ */
