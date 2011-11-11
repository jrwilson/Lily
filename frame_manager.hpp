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

/* Don't mess with memory below 1M or above 4G. */
const physical_address USABLE_MEMORY_BEGIN (0x00100000);
const physical_address USABLE_MEMORY_END (0xFFFFF000);

const int FRAME_SHIFT = 12;

struct page_directory_entry_t;
struct page_table_entry_t;

class frame
{
private:
  size_t f_;

public:
  frame () :
    f_ (0) { }

  explicit frame (const physical_address& address) :
    f_ (address.value () >> FRAME_SHIFT)
  { }

  explicit frame (const page_directory_entry_t& entry);

  explicit frame (const page_table_entry_t& entry);

  bool operator< (const frame& other) const {
    return f_ < other.f_;
  }

  bool operator>= (const frame& other) const {
    return f_ >= other.f_;
  }

  size_t operator- (const frame& other) const {
    return f_ - other.f_;
  }

  frame& operator+= (const size_t& offset) {
    f_ += offset;
    return *this;
  }

  operator physical_address () const {
    return physical_address (f_ << FRAME_SHIFT);
  }

  friend class page_table_entry_t;
  friend class page_directory_entry_t;
};

void
frame_manager_initialize (placement_allocator_t*);

void
frame_manager_add (placement_allocator_t*,
		   physical_address begin,
		   physical_address end);

/* This function allows a frame to be marked as used when initializing virtual memory. */
void
frame_manager_mark_as_used (frame frame);

/* Allocate a frame. */
frame
frame_manager_alloc () __attribute__((warn_unused_result));

/* Increment the reference count for a frame. */
void
frame_manager_incref (frame frame);

/* Decrement the reference count for a frame. */
void
frame_manager_decref (frame frame);

#endif /* __frame_manager_h__ */
