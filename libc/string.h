#ifndef STRING_H
#define STRING_H

#include <stddef.h>

extern int
memcmp (const void* s1,
	const void* s2,
	size_t n);

extern int
strcmp (const char* s1,
	const char* s2);

#endif /* STRING_H */
