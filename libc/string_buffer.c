#include "string_buffer.h"
#include "automaton.h"
#include "buffer_heap.h"

bd_t
string_buffer_harvest (string_buffer_t* sb,
		       size_t initial_capacity)
{
  bd_t retval = sb->bd;

  size_t total_size = sizeof (string_t) + initial_capacity;
  sb->bd = buffer_create (total_size);
  void* ptr = buffer_map (sb->bd);
  buffer_heap_t heap;
  buffer_heap_init (&heap, ptr, total_size);
  sb->string = buffer_heap_alloc (&heap, sizeof (string_t));
  sb->data = buffer_heap_alloc (&heap, initial_capacity);
  
  sb->string->size = 0;
  sb->string->capacity = initial_capacity;
  sb->string->data = (void*)sb->data - (void*)&sb->string->data;

  return retval;
}

void
string_buffer_put (string_buffer_t* sb,
		   unsigned char c)
{
  /* Resize the output array. */
  if (sb->string->size == sb->string->capacity) {
    size_t capacity = sb->string->capacity;
    buffer_unmap (sb->bd);
    buffer_grow (sb->bd, capacity);
    capacity *= 2;
    size_t total_size = sizeof (string_t) + capacity;
    void* ptr = buffer_map (sb->bd);
    buffer_heap_t heap;
    buffer_heap_init (&heap, ptr, total_size);
    sb->string = buffer_heap_alloc (&heap, sizeof (string_t));
    sb->string->capacity = capacity;
    sb->data = buffer_heap_alloc (&heap, capacity);
    /* Note:  The size and offset to the array haven't changed. */
  }
  
  /* Store the scan code. */
  sb->data[sb->string->size++] = c;
}

size_t
string_buffer_size (const string_buffer_t* sb)
{
  return sb->string->size;
}

void*
string_buffer_data (const string_buffer_t* sb)
{
  return sb->data;
}

bool
string_buffer_parse (string_buffer_t* sb,
		     bd_t bd,
		     void* ptr,
		     size_t size)
{
  sb->bd = bd;
  buffer_heap_t heap;
  buffer_heap_init (&heap, ptr, size);
  sb->string = buffer_heap_begin (&heap);
  if (buffer_heap_check (&heap, sb->string, sizeof (string_t))) {
    sb->data = (void*)&sb->string->data + sb->string->data;
    if (buffer_heap_check (&heap, sb->data, sb->string->capacity)) {
      return true;
    }
  }

  return false;
}
