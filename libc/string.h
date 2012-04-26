#ifndef STRING_H
#define STRING_H

#include <stddef.h>

typedef enum {
  STRING_SUCCESS,
  STRING_INVAL,
  STRING_RANGE,
  STRING_FORMAT,
} string_error_t;

extern string_error_t string_error;

int
pstrcmp (const char* s1_begin,
	 const char* s1_end,
	 const char* s2_begin,
	 const char* s2_end);

int
pstrncmp (const char* s1_begin,
	  const char* s1_end,
	  const char* s2_begin,
	  const char* s2_end,
	  size_t n);

char*
pstrchr (const char* s_begin,
	 const char* s_end,
	 int c);

long int
pstrtol (const char* begin,
	 const char* end,
	 char** endptr,
	 int base);

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

long int
strtol (const char* nptr,
	char** endptr,
	int base);

unsigned long int
strtoul (const char* nptr,
	 char** endptr,
	 int base);

int
snprintf (char* str,
	  size_t size,
	  const char* format,
	  ...);

#endif /* STRING_H */
