/*
  File
  ----
  hash_map.c
  
  Description
  -----------
  A simple hash map implementation.

  Authors
  -------
  Justin R. Wilson
*/

#include "hash_map.h"
#include "kassert.h"

#define HASH_LOAD_NUMERATOR 7
#define HASH_LOAD_DENOMINATOR 10

typedef struct hash_map_node hash_map_node_t;
struct hash_map_node {
  hash_map_node_t* next;
  size_t hash;
  const void* key;
  void* value;
};

struct hash_map {
  list_allocator_t* list_allocator;
  size_t size;
  size_t capacity;
  hash_map_node_t** hash;
  hash_map_hash_func_t hash_func;
  hash_map_compare_func_t compare_func;
};

static hash_map_node_t*
allocate_hash_map_node (list_allocator_t* la,
			size_t hash,
			const void* key,
			void* value)
{
  hash_map_node_t* ptr = static_cast<hash_map_node_t*> (list_allocator_alloc (la, sizeof (hash_map_node_t)));
  ptr->next = 0;
  ptr->hash = hash;
  ptr->key = key;
  ptr->value = value;
  return ptr;
}

static void
free_hash_map_node (list_allocator_t* la,
		    hash_map_node_t* ptr)
{
  kassert (ptr != 0);
  list_allocator_free (la, ptr);
}

hash_map_t*
hash_map_allocate (list_allocator_t* la,
		   hash_map_hash_func_t hash_func,
		   hash_map_compare_func_t compare_func)
{
  hash_map_t* ptr = static_cast<hash_map_t*> (list_allocator_alloc (la, sizeof (hash_map_t)));
  ptr->list_allocator = la;
  ptr->size = 0;
  ptr->capacity = 1;
  ptr->hash = static_cast<hash_map_node_t**> (list_allocator_alloc (la, ptr->capacity * sizeof (hash_map_node_t*)));
  size_t idx;
  for (idx = 0; idx < ptr->capacity; ++idx) {
    ptr->hash[idx] = 0;
  }
  ptr->hash_func = hash_func;
  ptr->compare_func = compare_func;
  return ptr;
}

size_t
hash_map_size (const hash_map_t* ptr)
{
  kassert (ptr != 0);
  return ptr->size;
}

void
hash_map_insert (hash_map_t* ptr,
		 const void* key,
		 void* value)
{
  kassert (ptr != 0);

  if (HASH_LOAD_DENOMINATOR * ptr->size > HASH_LOAD_NUMERATOR * ptr->capacity) {
    size_t new_capacity = ptr->capacity * 2;
    hash_map_node_t** new_hash = static_cast<hash_map_node_t**> (list_allocator_alloc (ptr->list_allocator, new_capacity * sizeof (hash_map_node_t*)));
    size_t idx;
    for (idx = 0; idx < new_capacity; ++idx) {
      new_hash[idx] = 0;
    }
    for (idx = 0; idx < ptr->capacity; ++idx) {
      while (ptr->hash[idx] != 0) {
      	hash_map_node_t* n = ptr->hash[idx];
      	ptr->hash[idx] = n->next;
      	n->next = new_hash[n->hash % new_capacity];
      	new_hash[n->hash % new_capacity] = n;
      }
    }
    list_allocator_free (ptr->list_allocator, ptr->hash);
    ptr->hash = new_hash;
    ptr->capacity = new_capacity;
  }

  size_t hash = ptr->hash_func (key);
  size_t hash_idx = hash % ptr->capacity;
  hash_map_node_t* node = allocate_hash_map_node (ptr->list_allocator, hash, key, value);
  node->next = ptr->hash[hash_idx];
  ptr->hash[hash_idx] = node;
  ++ptr->size;
}

void
hash_map_erase (hash_map_t* ptr,
		const void* key)
{
  kassert (ptr != 0);

  size_t hash = ptr->hash_func (key);
  size_t hash_idx = hash % ptr->capacity;

  hash_map_node_t** n = &ptr->hash[hash_idx];
  for (; (*n) != 0 && !((*n)->hash == hash && ptr->compare_func ((*n)->key, key) == 0); n = &((*n)->next)) ;;
  if ((*n) != 0) {
    hash_map_node_t* temp = *n;
    *n = temp->next;
    free_hash_map_node (ptr->list_allocator, temp);
    --ptr->size;
  }
}

int
hash_map_contains (const hash_map_t* ptr,
		   const void* key)
{
  kassert (ptr != 0);

  size_t hash = ptr->hash_func (key);
  size_t hash_idx = hash % ptr->capacity;

  hash_map_node_t* n;
  for (n = ptr->hash[hash_idx]; n != 0 && !(n->hash == hash && ptr->compare_func (n->key, key) == 0); n = n->next) ;;

  return n != 0;
}

void*
hash_map_find (const hash_map_t* ptr,
	       const void* key)
{
  kassert (ptr != 0);

  size_t hash = ptr->hash_func (key);
  size_t hash_idx = hash % ptr->capacity;

  hash_map_node_t* n = ptr->hash[hash_idx];
  for (; n != 0 && !(n->hash == hash && ptr->compare_func (n->key, key) == 0); n = n->next) ;;
  if (n != 0) {
    return n->value;
  }
  else {
    return 0;
  }
}

void
hash_map_dump (const hash_map_t* ptr,
	       void (*key_func) (const void*),
	       void (*value_func) (const void*))
{
  kassert (ptr != 0);

  size_t k;
  for (k = 0; k < ptr->capacity; ++k) {
    kputs ("hash index = "); kputx32 (k); kputs ("\n");
    hash_map_node_t* n;
    for (n = ptr->hash[k]; n != 0; n = n->next) {
      kputs ("hash = "); kputx32 (n->hash);
      kputs (" hash_idx = "); kputx32 (n->hash % ptr->capacity);
      if (key_func) {
	key_func (n->key);
      }
      if (value_func) {
	value_func (n->value);
      }
      kputs ("\n");
    }
  }
}
