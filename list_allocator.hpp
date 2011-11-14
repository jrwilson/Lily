#ifndef __list_allocator_hpp__
#define __list_allocator_hpp__

/*
  File
  ----
  list_allocator.hpp
  
  Description
  -----------
  Memory allocator using a doubly-linked list.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"

class list_allocator {
private:
  struct chunk_header;
  
  size_t page_size_;
  chunk_header* first_header_;
  chunk_header* last_header_;

  static chunk_header*
  find_header (chunk_header* start,
	       size_t size);

  void
  split_header (chunk_header* ptr,
		size_t size);

public:
  list_allocator ();
  void* operator new (size_t);
  void operator delete (void*);

  void*
  alloc (size_t) __attribute__((warn_unused_result));
  
  void
  free (void*);
};


#endif /* __list_allocator_hpp__ */
