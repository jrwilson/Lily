#ifndef __string_h__
#define __string_h__

/*
  File
  ----
  string.h
  
  Description
  -----------
  Functions for working with strings.

  Authors:
  Justin R. Wilson
*/

#include <stddef.h>

char*
strcpy (char* p,
	const char* q);

char*
strcat (char* p,
	const char* q);

char*
strncpy (char* p,
	 const char* q,
	 int n);

char*
strncat (char* p,
	 const char* q,
	 int n);

size_t
strlen (const char* p);

int
strcmp (const char* p,
	const char* q);

int
strncmp (const char* p,
	 const char* q,
	 int n);

char*
strchr (char* p,
	int c);

const char*
strchr (const char* p,
	int c);

char*
strrchr (char* p,
	 int c);

const char*
strrchr (const char* p,
	 int c);

char*
strstr (char* p,
	const char* q);


char*
strpbrk (char* p,
	 const char* q);

const char*
strpbrk (const char* p,
	 const char* q);

size_t
strspn (const char* p,
	const char* q);

size_t
strcspn (const char* p,
	 const char* q);

void*
memcpy (void* dst,
	const void* src,
	size_t size);

void*
memmove (void* dst,
	 const void* src,
	 size_t size);

void*
memchr (const void* p,
	int b,
	size_t n);

int
memcmp (const void* p,
	const void* q,
	size_t n);

void*
memset (void* ptr,
	int b,
	size_t size);

#endif /* __string_h__ */

