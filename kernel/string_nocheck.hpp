#ifndef __string_nocheck_hpp__
#define __string_nocheck_hpp__

/* GCC is both a curse and a blessing.
   On one hand, they have a great C++ Standard Template Library implementation.
   Currently, I'm forced to use the option -std=c++0x because I use unordered sets, etc.
   The curse is that doing so brings in a number of dependencies, e.g., RTTI, and causes a number of feature test macros to be defined, e.g., __USE_BSD, __USE_FORTIFY_SOURCE.
   Basically, its not modular.

   This particular file exists due to the inability to not __USE_FORTIFY_SOURCE.
   This option turns on error checking in string functions which fails when manipulating regions of memory that exist outside of nice and cozy C environment.
 */

inline void*
memset_nocheck (void* ptr,
		int value,
		size_t size)
{
  unsigned char* p = static_cast<unsigned char*> (ptr);
  while (size-- > 0) {
    *p++ = value;
  }
  return ptr;
}

inline void*
memcpy_nocheck (void* dst,
		const void* src,
		size_t size)
{
  unsigned char* d = static_cast<unsigned char*> (dst);
  const unsigned char* s = static_cast<const unsigned char*> (src);
  while (size-- != 0) {
    *d++ = *s++;
  }
  return dst;
}

#endif /* __string_nocheck_hpp__ */
