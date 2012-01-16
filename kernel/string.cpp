#include "string_nocheck.hpp"
#include "kassert.hpp"

// TODO:  Clean up.

// void*
// memcpy (void* dst,
// 	const void* src,
// 	size_t size)
// {
//   return memcpy_nocheck (dst, src, size);
// }

// void*
// memmove (void* dest,
// 	 const void* src,
// 	 size_t n)
// {
//   unsigned char* d = static_cast<unsigned char*> (dest);
//   const unsigned char* s = static_cast<const unsigned char*> (src);

//   if (s >= d || s + n <= d) {
//     // No overlap.
//     memcpy (dest, src, n);
//   }
//   else {
//     s += n;
//     d += n;
//     while (n-- != 0) {
//       *(--d) = *(--s);
//     }
//   }
  
//   return dest;
// }

// int
// strncmp (const char* p,
// 	 const char* q,
// 	 size_t n)
// {
//   while (n != 0 && *p != 0 && *q != 0 && *p == *q) {
//     ++p;
//     ++q;
//     --n;
//   }
    
//   return (n == 0) ? 0 : *p - *q;
// }

// int
// memcmp (const void* p,
// 	const void* q,
// 	size_t n)
// {
//   const char* r = static_cast<const char*> (p);
//   const char* s = static_cast<const char*> (q);
  
//   for (; n != 0; --n, ++r, ++s) {
//     if (*r != *s) {
//       return *r - *s;
//     }
//   }
  
//   return 0;
// }

// const void*
// memchr (const void* s,
// 	int c,
// 	size_t n)
// {
//   const unsigned char* str = static_cast<const unsigned char*> (s);
//   for (; n != 0; --n, ++str) {
//     if (*str == c) {
//       return str;
//     }
//   }

//   return 0;
// }

// extern "C" void*
// __memcpy_chk (void* dst,
// 	      const void* src,
// 	      size_t len,
// 	      size_t destlen)
// {
//   if (len > destlen) {
//     // Error.
//     kassert (0);
//   }
//   return memcpy_nocheck (dst, src, len);
// }

// extern "C" void*
// __memset_chk (void* dest,
// 	      int value,
// 	      size_t len,
// 	      size_t destlen)
// {
//   if (len > destlen) {
//     // Error.
//     kassert (0);
//   }
//   return memset_nocheck (dest, value, len);
// }




// inline char*
// strcpy (char* p,
// 	const char* q)
// {
//   char* retval = p;
//   while (*q != 0) {
//     *p++ = *q++;
//   }
//   return retval;
// }
