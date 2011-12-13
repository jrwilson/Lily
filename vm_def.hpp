#ifndef __vm_def_hpp__
#define __vm_def_hpp__

#include "types.hpp"

typedef size_t frame_t;
typedef size_t physical_address_t;

/* Don't mess with memory below 1M or above 4G. */
static const physical_address_t USABLE_MEMORY_BEGIN = 0x00100000;
static const physical_address_t USABLE_MEMORY_END = 0xFFFFF000;

/* Should agree with loader.s. */
static const void* const KERNEL_VIRTUAL_BASE = reinterpret_cast<const void*> (0xC0000000);

/* Everything must fit into 4MB initialy. */
static const void* const INITIAL_LOGICAL_LIMIT = reinterpret_cast<const void*> (0xC0400000);

// One megabyte is important for dealing with low memory.
static const size_t ONE_MEGABYTE = 0x00100000;

// Four megabytes are important because that is span of one page table or page directory entry.
static const size_t FOUR_MEGABYTES = 0x00400000;

/* Logical address above this address are using for page tables. */
static const void* const PAGING_AREA = reinterpret_cast<const void*> (0xFFC00000);

#define PAGE_SIZE 0x1000

static const int FRAME_SHIFT = 12;

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

inline void*
align_down (void* address,
	    size_t radix)
{
  return reinterpret_cast<void*> (reinterpret_cast<size_t> (address) & ~(radix - 1));
}

inline void*
align_up (void* address,
	  size_t radix)
{
  return reinterpret_cast<void*> ((reinterpret_cast<size_t> (address) + radix - 1) & ~(radix - 1));
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

inline const void*
get_address (size_t directory_entry,
	     size_t table_entry)
{
  return reinterpret_cast<const void*> (directory_entry << 22 | table_entry << 12);
}

#endif /* __vm_def_hpp__ */
