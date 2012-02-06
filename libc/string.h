#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int
memcmp (const void* s1,
	const void* s2,
	size_t n);

void*
memcpy (void* dest,
	const void* src,
	size_t n);

void*
memmove (void* dest,
	 const void* src,
	 size_t n);

void*
memset (void* s,
	int c,
	size_t n);

int
strcmp (const char* s1,
	const char* s2);

size_t
strlen (const char* s);

int
snprintf (char* str,
	  size_t size,
	  const char* format,
	  ...);

#endif /* STRING_H */
