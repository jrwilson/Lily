#ifndef __hash_map_h__
#define __hash_map_h__

/*
  File
  ----
  hash_map.h
  
  Description
  -----------
  A simple hash map implemntation.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"
#include "list_allocator.hpp"

typedef struct hash_map hash_map_t;
typedef size_t (*hash_map_hash_func_t) (const void*);
typedef int (*hash_map_compare_func_t) (const void*, const void*);

hash_map_t*
hash_map_allocate (list_allocator_t* la,
		   hash_map_hash_func_t hash_func,
		   hash_map_compare_func_t compare_func);

size_t
hash_map_size (const hash_map_t* ptr);

void
hash_map_insert (hash_map_t* ptr,
		 const void* key,
		 void* value);

void
hash_map_erase (hash_map_t* ptr,
		const void* key);

int
hash_map_contains (const hash_map_t* ptr,
		   const void* key);

void*
hash_map_find (const hash_map_t* ptr,
	       const void* key);

void
hash_map_dump (const hash_map_t* ptr,
	       void (*key_func) (const void*),
	       void (*value_func) (const void*));

#endif /* __hash_map_h__ */
