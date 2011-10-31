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
#include "memory.h"
#include "kassert.h"

#define HASH_LOAD_NUMERATOR 7
#define HASH_LOAD_DENOMINATOR 10

typedef struct hash_map_node hash_map_node_t;
struct hash_map_node {
  hash_map_node_t* next;
  unsigned int hash;
  const void* key;
  void* value;
};

struct hash_map {
  unsigned int size;
  unsigned int capacity;
  hash_map_node_t** hash;
  hash_map_hash_func_t hash_func;
  hash_map_compare_func_t compare_func;
};

static hash_map_node_t*
allocate_hash_map_node (unsigned int hash,
			const void* key,
			void* value)
{
  hash_map_node_t* ptr = kmalloc (sizeof (hash_map_node_t));
  ptr->next = 0;
  ptr->hash = hash;
  ptr->key = key;
  ptr->value = value;
  return ptr;
}

static void
free_hash_map_node (hash_map_node_t* ptr)
{
  kassert (ptr != 0);
  kfree (ptr);
}

hash_map_t*
allocate_hash_map (hash_map_hash_func_t hash_func,
		   hash_map_compare_func_t compare_func)
{
  hash_map_t* ptr = kmalloc (sizeof (hash_map_t));
  ptr->size = 0;
  ptr->capacity = 1;
  ptr->hash = kmalloc (ptr->capacity * sizeof (hash_map_node_t*));
  unsigned int idx;
  for (idx = 0; idx < ptr->capacity; ++idx) {
    ptr->hash[idx] = 0;
  }
  ptr->hash_func = hash_func;
  ptr->compare_func = compare_func;
  return ptr;
}

unsigned int
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
    unsigned int new_capacity = ptr->capacity * 2;
    hash_map_node_t** new_hash = kmalloc (new_capacity * sizeof (hash_map_node_t*));
    unsigned int idx;
    for (idx = 0; idx < new_capacity; ++idx) {
      new_hash[idx] = 0;
    }
    for (idx = 0; idx < ptr->capacity; ++idx) {
      while (ptr->hash[idx] != 0) {
      	hash_map_node_t* n = ptr->hash[idx];
      	ptr->hash[idx] = n->next;
      	n->next = new_hash[n->hash % ptr->capacity];
      	new_hash[n->hash % ptr->capacity] = n;
      }
    }
    kfree (ptr->hash);
    ptr->hash = new_hash;
    ptr->capacity = new_capacity;
  }

  unsigned int hash = ptr->hash_func (key);
  unsigned int hash_idx = hash % ptr->capacity;
  hash_map_node_t* node = allocate_hash_map_node (hash, key, value);
  node->next = ptr->hash[hash_idx];
  ptr->hash[hash_idx] = node;
  ++ptr->size;
}

void
hash_map_erase (hash_map_t* ptr,
		const void* key)
{
  kassert (ptr != 0);

  unsigned int hash = ptr->hash_func (key);
  unsigned int hash_idx = hash % ptr->capacity;

  hash_map_node_t** n = &ptr->hash[hash_idx];
  for (; (*n) != 0 && !((*n)->hash == hash && ptr->compare_func ((*n)->key, key) == 0); n = &((*n)->next)) ;;
  if ((*n) != 0) {
    hash_map_node_t* temp = *n;
    *n = temp->next;
    free_hash_map_node (temp);
    --ptr->size;
  }
}

int
hash_map_contains (hash_map_t* ptr,
		   const void* key)
{
  kassert (ptr != 0);

  unsigned int hash = ptr->hash_func (key);
  unsigned int hash_idx = hash % ptr->capacity;

  hash_map_node_t* n = ptr->hash[hash_idx];
  for (; n != 0 && !(n->hash == hash && ptr->compare_func (n->key, key) == 0); n = n->next) ;;

  return n != 0;
}

void*
hash_map_find (hash_map_t* ptr,
	       const void* key)
{
  kassert (ptr != 0);

  unsigned int hash = ptr->hash_func (key);
  unsigned int hash_idx = hash % ptr->capacity;

  hash_map_node_t* n = ptr->hash[hash_idx];
  for (; n != 0 && !(n->hash == hash && ptr->compare_func (n->key, key) == 0); n = n->next) ;;
  if (n != 0) {
    return n->value;
  }
  else {
    return 0;
  }
}
