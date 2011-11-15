#ifndef __new_hpp__
#define __new_hpp__

/*
  File
  ----
  new.hpp
  
  Description
  -----------
  Functions for dynamic memory.

  Authors:
  Justin R. Wilson
*/

#include "list_allocator.hpp"

inline void*
operator new (size_t sz)
{
  return list_allocator::alloc (sz);
}

inline void
operator delete (void* p)
{
  list_allocator::free (p);
}

inline void*
operator new[] (size_t sz)
{
  return list_allocator::alloc (sz);
}

inline void
operator delete[] (void* p)
{
  list_allocator::free (p);
}

inline void*
operator new (size_t,
	      void* p)
{
  return p;
}

inline void
operator delete (void*,
		 void*)
{
  // Do nothing.
}

inline void*
operator new[] (size_t,
		void* p)
{
  return p;
}

inline void
operator delete[] (void*,
		   void*)
{
  // Do nothing.
}

#endif /* __new_hpp_ */
