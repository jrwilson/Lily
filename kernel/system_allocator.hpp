#ifndef __system_allocator_hpp__
#define __system_allocator_hpp__

/*
  File
  ----
  system_allocator.hpp
  
  Description
  -----------
  Memory allocator using a doubly-linked list and no system calls.

  Authors:
  Justin R. Wilson
*/

#include "list_allocator.hpp"
#include "vm_def.hpp"
#include "kassert.hpp"

class system_syscall {
private:
  static logical_address_t heap_begin_;
  static logical_address_t heap_end_;
  static logical_address_t heap_limit_;
  static bool backing_;
  static list_alloc_state state_;

public:
  static void
  initialize (logical_address_t begin,
	      logical_address_t limit)
  {
    // We use a page as the initial size of the heap.
    kassert (begin < limit);
    kassert (limit - begin >= PAGE_SIZE);

    heap_begin_ = begin;
    heap_end_ = begin;
    heap_limit_ = limit;
    backing_ = false;
  }

  static logical_address_t
  heap_begin ()
  {
    return heap_begin_;
  }
  
  static logical_address_t
  heap_end ()
  {
    return heap_end_;
  }

  static void
  engage_vm (logical_address_t limit)
  {
    heap_limit_ = limit;
    backing_ = true;
  }

  static void*
  sbrk (size_t size);

  static size_t
  getpagesize ()
  {
    return PAGE_SIZE;
  }

  static inline list_alloc_state&
  state ()
  {
    return state_;
  }
};

class system_alloc : public list_alloc<system_syscall> { };

template <typename T>
struct system_allocator : public list_allocator<T, system_syscall> { };

#endif /* __system_allocator_hpp__ */
