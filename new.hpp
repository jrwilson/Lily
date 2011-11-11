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

inline void* operator new (size_t,
			   void* p) throw ()
{
  return p;
}

#endif /* __new_hpp_ */
