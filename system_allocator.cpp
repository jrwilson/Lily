/*
  File
  ----
  system_allocator.cpp
  
  Description
  -----------
  Memory allocator using a doubly-linked list.

  Authors:
  Justin R. Wilson
*/

#include "system_allocator.hpp"
#include "frame_manager.hpp"
#include "vm.hpp"

logical_address_t system_sbrk::heap_begin_ = 0;
logical_address_t system_sbrk::heap_end_ = 0;
logical_address_t system_sbrk::heap_limit_ = 0;
bool system_sbrk::backing_ = false;

template <typename T1, typename T2>
typename list_alloc<T1, T2>::data list_alloc<T1, T2>::data_;

void*
system_sbrk::operator() (size_t size)
{
  // Page aligment makes mapping easier.
  kassert (is_aligned (size, PAGE_SIZE));
  logical_address_t retval = heap_end_;
  heap_end_ += size;
  // Check to make sure we don't run out of logical address space.
  kassert (heap_end_ <= heap_limit_);
  if (backing_) {
    // Back with frames.
    for (size_t x = 0; x != size; x += PAGE_SIZE) {
      vm::map (retval + x, frame_manager::alloc (), vm::USER, vm::WRITABLE);
    }
  }
  
  return reinterpret_cast<void*> (retval);
}
