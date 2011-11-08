/*
  File
  ----
  frame_manager.c
  
  Description
  -----------
  The frame manager manages physical memory.

  Authors:
  Justin R. Wilson
*/

#include "frame_manager.h"
#include "vm_manager.h"
#include "kassert.h"

/* Don't mess with memory below 1M or above 4G. */
#define MEMORY_LOWER_LIMIT 0x00100000
#define MEMORY_UPPER_LIMIT 0xFFFFFFFF

/*
  The frame manager was designed under the following requirements and assumptions:
  1.  All frames can be shared.
  2.  The physical location of a frame doesn't matter.

  Sharing frames implies reference counting.

  The assumption that all frames can be shared is somewhat naive as only frames containing code and buffers can be shared.
  Thus, an optimization is to assume that only a fixed fraction of frames are shared.

  Assuming that the physical location of a frame will probably break down once we start doing DMA.
  However, this should only require expanding the interface and changing the implementation as opposed to redesigning the interface.
 */

/*
  A frame entry stores the next entry on the free list or the reference count.
  Next has the range [0, size - 1].
  STACK_ALLOCATOR_EOL marks the end of the list.
  Reference count is the additive inverse of the reference count.

  The goal is to support 4GB of memory.
  4GB / PAGE_SIZE = 1,048,576 frames.
  The two choices are to use 1 table with 31-bit indexing or multiple tables with 15-bit indexing.
  The space required to hold the table entries is:
  1,048,576 frames * 4 bytes/frame = 4MB
  1,048,576 frames * 2 bytes/frame = 2MB
  The non-entry overhead is negligible, i.e., propotional to the number of regions multiplied by sizeof (stack_allocator_t).

  With 31-bit entries, a frame can be shared 2,147,483,647 times.
  With 15-bit entries, a frame can be shared 32,767 times.

  Share a page 32,767 times seems reasonable so I will use the more space-efficient 15-bit entries.
*/
typedef int16_t frame_entry_t;
#define STACK_ALLOCATOR_EOL -32768
#define MAX_REGION_SIZE 0x07FFF000

typedef struct stack_allocator stack_allocator_t;
struct stack_allocator {
  stack_allocator_t* next;
  frame_t begin;
  frame_t end;
  frame_entry_t free_head;
  frame_entry_t entry[];
};

static placement_allocator_t* placement_allocator = 0;
static stack_allocator_t* stack_allocators = 0;

static stack_allocator_t*
stack_allocator_allocate (frame_t begin,
			  frame_t end)
{
  kassert (begin < end);

  frame_entry_t size = end - begin;
  stack_allocator_t* ptr = placement_allocator_alloc (placement_allocator, sizeof (stack_allocator_t) + size * sizeof (frame_entry_t));
  kassert (ptr != 0);
  ptr->next = 0;
  ptr->begin = begin;
  ptr->end = end;
  ptr->free_head = 0;
  frame_entry_t k;
  for (k = 0; k < size; ++k) {
    ptr->entry[k] = k + 1;
  }
  ptr->entry[size - 1] = STACK_ALLOCATOR_EOL;

  return ptr;
}

static void
stack_allocator_mark_as_used (stack_allocator_t* ptr,
			      uint32_t frame)
{
  kassert (ptr != 0);
  kassert (frame >= ptr->begin && frame < ptr->end);

  frame_entry_t frame_idx = frame - ptr->begin;
  /* Should be free. */
  kassert (ptr->entry[frame_idx] >= 0 && ptr->entry[frame_idx] != STACK_ALLOCATOR_EOL);
  /* Find the one that points to it. */
  int idx;
  for (idx = ptr->free_head; idx != STACK_ALLOCATOR_EOL && ptr->entry[idx] != frame_idx; idx = ptr->entry[idx]) ;;
  kassert (idx != STACK_ALLOCATOR_EOL && ptr->entry[idx] == frame_idx);
  /* Update the pointers. */
  ptr->entry[idx] = ptr->entry[frame_idx];
  ptr->entry[frame_idx] = -1;
}

static uint32_t
stack_allocator_alloc (stack_allocator_t* ptr)
{
  kassert (ptr != 0);
  kassert (ptr->free_head != STACK_ALLOCATOR_EOL);

  frame_entry_t idx = ptr->free_head;
  ptr->free_head = ptr->entry[idx];
  ptr->entry[idx] = -1;
  return ptr->begin + idx;
}

static void
stack_allocator_incref (stack_allocator_t* ptr,
			uint32_t frame)
{
  kassert (ptr != 0);
  kassert (frame >= ptr->begin && frame < ptr->end);
  frame_entry_t idx = frame - ptr->begin;
  /* Frame is allocated. */
  kassert (ptr->entry[idx] < 0);
  /* "Increment" the reference count. */
  --ptr->entry[idx];
}

static void
stack_allocator_decref (stack_allocator_t* ptr,
			uint32_t frame)
{
  kassert (ptr != 0);
  kassert (frame >= ptr->begin && frame < ptr->end);
  frame_entry_t idx = frame - ptr->begin;
  /* Frame is allocated. */
  kassert (ptr->entry[idx] < 0);
  /* "Decrement" the reference count. */
  ++ptr->entry[idx];
  /* Free the frame. */
  if (ptr->entry[idx]) {
    ptr->entry[idx] = ptr->free_head;
    ptr->free_head = idx;
  }
}

void
frame_manager_initialize (placement_allocator_t* p_a)
{
  kassert (placement_allocator == 0);
  kassert (p_a != 0);
  placement_allocator = p_a;
}

void
frame_manager_add (placement_allocator_t* p_a,
		   uint64_t first,
		   uint64_t last)
{
  kassert (p_a == placement_allocator);
  kassert (first <= last);

  /* Intersect [first, last] with [MEMORY_LOWER_LIMIT, MEMORY_UPPER_LIMIT]. */
  first = (first > MEMORY_LOWER_LIMIT) ? first : MEMORY_LOWER_LIMIT;
  last = (last < MEMORY_UPPER_LIMIT) ? last : MEMORY_UPPER_LIMIT;

  /* Align to frame boundaries. */
  first = PAGE_ALIGN_UP (first);
  last = PAGE_ALIGN_DOWN (last + 1) - 1;

  if (first < last) {
    uint64_t size = last - first + 1;
    while (size != 0) {
      uint64_t sz = (size >= MAX_REGION_SIZE) ? MAX_REGION_SIZE : size;
      stack_allocator_t* stack_allocator = stack_allocator_allocate (ADDRESS_TO_FRAME (first), ADDRESS_TO_FRAME (first + sz));
      stack_allocator->next = stack_allocators;
      stack_allocators = stack_allocator;
      size -= sz;
      first += sz;
    }
  }
}

static stack_allocator_t*
find_allocator (uint32_t frame)
{
  stack_allocator_t* ptr;
  /* Find the allocator. */
  for (ptr = stack_allocators; ptr != 0 && !(frame >= ptr->begin && frame < ptr->end) ; ptr = ptr->next) ;;

  return ptr;
}

void
frame_manager_mark_as_used (uint32_t frame)
{
  stack_allocator_t* ptr = find_allocator (frame);

  if (ptr != 0) {
    stack_allocator_mark_as_used (ptr, frame);
  }
}

uint32_t
frame_manager_alloc ()
{
  stack_allocator_t* ptr;
  /* Find an allocator with a free frame. */
  for (ptr = stack_allocators; ptr != 0 && ptr->free_head == STACK_ALLOCATOR_EOL; ptr = ptr->next) ;;

  /* Out of frames. */
  kassert (ptr != 0);

  return stack_allocator_alloc (ptr);
}

void
frame_manager_incref (uint32_t frame)
{
  stack_allocator_t* ptr = find_allocator (frame);

  /* No allocator for frame. */
  kassert (ptr != 0);

  stack_allocator_incref (ptr, frame);
}

void
frame_manager_decref (uint32_t frame)
{
  stack_allocator_t* ptr = find_allocator (frame);

  /* No allocator for frame. */
  kassert (ptr != 0);

  stack_allocator_decref (ptr, frame);
}
