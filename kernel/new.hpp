#ifndef __new_hpp__
#define __new_hpp__

inline void*
operator new (size_t,
	      void* ptr)
{
  return ptr;
}

#endif /* __new_hpp__ */
