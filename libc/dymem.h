#ifndef DYMEM_H
#define DYMEM_H

#include <stddef.h>

void*
malloc (size_t size);

void
free (void* ptr);

void*
realloc (void* ptr,
	 size_t size);

#endif /* DYMEM_H */
