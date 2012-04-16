#ifndef STRING_H
#define STRING_H

#include <stddef.h>

char*
strcpy (char* dest,
	const char* src);

char*
strncpy (char* dest,
	 const char* src,
	 size_t n);

char*
strcat (char* dest,
	const char* src);

char*
strncat (char* dest,
	 const char* src,
	 size_t n);

/* strxfrm not implemented. */

size_t
strlen (const char* s);

int
strcmp (const char* s1,
	const char* s2);

int
strncmp (const char* s1,
	 const char* s2,
	 size_t n);

/* strcoll not implemented. */

char*
strchr (const char* s,
	int c);

char*
strrchr (const char* s,
	 int c);

size_t
strspn (const char* s,
	const char* accept);

size_t
strcspn (const char* s,
	 const char* reject);

char*
strpbrk (const char* s,
	 const char* accept);

/* strtok not implemented. */

/* strerror not implemented. */

void*
memset (void* s,
	int c,
	size_t n);

void*
memcpy (void* dest,
	const void* src,
	size_t n);

void*
memmove (void* dest,
	 const void* src,
	 size_t n);

int
memcmp (const void* s1,
	const void* s2,
	size_t n);

void*
memchr (const void *s,
	int c,
	size_t n);

#endif /* STRING_H */
