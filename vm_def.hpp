#ifndef __vm_def_hpp__
#define __vm_def_hpp__

#include "types.hpp"

#define PAGE_SIZE 0x1000

/* Should agree with loader.s. */
static const void* KERNEL_VIRTUAL_BASE = reinterpret_cast<const void*> (0xC0000000);

/* Everything must fit into 4MB initialy. */
static const void* INITIAL_LOGICAL_LIMIT = reinterpret_cast<const void*> (0xC0400000);

/* Memory under one megabyte is not used. */
static const void* ONE_MEGABYTE = reinterpret_cast<const void*> (0x00100000);

static const int FRAME_SHIFT = 12;

typedef size_t frame_t;
typedef size_t physical_address_t;

inline frame_t
physical_address_to_frame (physical_address_t address)
{
  return address >> FRAME_SHIFT;
}

inline physical_address_t
frame_to_physical_address (frame_t frame)
{
  return frame << FRAME_SHIFT;
}

inline physical_address_t
align_down (physical_address_t address,
	    size_t radix)
{
  return address & ~(radix - 1);
}

inline physical_address_t
align_up (physical_address_t address,
	  size_t radix)
{
  return (address + radix - 1) & ~(radix - 1);
}

inline bool
is_aligned (physical_address_t address,
	    size_t radix)
{
  return (address & (radix - 1)) == 0;
}

inline const void*
align_down (const void* address,
	    size_t radix)
{
  return reinterpret_cast<const void*> (reinterpret_cast<size_t> (address) & ~(radix - 1));
}

inline const void*
align_up (const void* address,
	  size_t radix)
{
  return reinterpret_cast<const void*> ((reinterpret_cast<size_t> (address) + radix - 1) & ~(radix - 1));
}

inline bool
is_aligned (const void* address,
	    size_t radix)
{
  return (reinterpret_cast<size_t> (address) & (radix - 1)) == 0;
}

inline physical_address_t
logical_to_physical (const void* address,
		     const void* offset)
{
  return reinterpret_cast<physical_address_t> (address) - reinterpret_cast<physical_address_t> (offset);
}

inline size_t
get_page_table_entry (const void* address)
{
  return (reinterpret_cast<size_t> (address) & 0x3FF000) >> 12;
}

inline size_t
get_page_directory_entry (const void* address)
{
  return (reinterpret_cast<size_t> (address) & 0xFFC00000) >> 22;
}

#endif /* __vm_def_hpp__ */
