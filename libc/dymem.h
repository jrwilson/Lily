#ifndef DYMEM_H
#define DYMEM_H

#include <stddef.h>
#include <lily/types.h>

void*
malloc (lily_error_t* err,
	size_t size);

void
free (void* ptr);

void*
realloc (lily_error_t* err,
	 void* ptr,
	 size_t size);

#endif /* DYMEM_H */
