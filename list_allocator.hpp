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
  
  static size_t page_size_;
  static chunk_header* first_header_;
  static chunk_header* last_header_;

  static chunk_header*
  find_header (chunk_header* start,
	       size_t size);

  static void
  split_header (chunk_header* ptr,
		size_t size);

public:
  static void*
  alloc (size_t) __attribute__((warn_unused_result));
  
  static void
  free (void*);
};


#endif /* __list_allocator_hpp__ */
