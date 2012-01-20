#ifndef __vm_def_hpp__
#define __vm_def_hpp__

#include <stddef.h>
#include "integer_types.hpp"

typedef uintptr_t physical_address_t;
typedef uintptr_t logical_address_t;
typedef uintptr_t frame_t;

typedef size_t page_table_idx_t;

static const size_t PAGE_SIZE = 0x1000;

/* Number of entries in a page table or directory. */
static const page_table_idx_t PAGE_ENTRY_COUNT = 1024;

static const int FRAME_SHIFT = 12;

/* Don't mess with memory below 1M or above 4G. */
static const physical_address_t USABLE_MEMORY_BEGIN = 0x00100000;
static const physical_address_t USABLE_MEMORY_END = 0xFFFFF000;

/* Should agree with loader.s. */
static const logical_address_t KERNEL_VIRTUAL_BASE = 0xC0000000;

/* Everything must fit into 4MB initialy. */
static const logical_address_t INITIAL_LOGICAL_LIMIT = 0xC0400000;

// One megabyte is important for dealing with low memory.
static const size_t ONE_MEGABYTE = 0x00100000;

// Four megabytes are important because that is span of one page table or page directory entry.
static const size_t FOUR_MEGABYTES = 0x00400000;

/* Logical address above this address are using for page tables. */
static const logical_address_t PAGING_AREA = 0xFFC00000;

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
	    unsigned int radix)
{
  return address & ~(radix - 1);
}

inline physical_address_t
align_up (physical_address_t address,
	  unsigned int radix)
{
  return (address + radix - 1) & ~(radix - 1);
}

inline bool
is_aligned (physical_address_t address,
	    unsigned int radix)
{
  return (address & (radix - 1)) == 0;
}

inline const void*
align_down (const void* address,
	    unsigned int radix)
{
  return reinterpret_cast<const void*> (reinterpret_cast<physical_address_t> (address) & ~(radix - 1));
}

inline const void*
align_up (const void* address,
	  unsigned int radix)
{
  return reinterpret_cast<const void*> ((reinterpret_cast<physical_address_t> (address) + radix - 1) & ~(radix - 1));
}

inline void*
align_down (void* address,
	    unsigned int radix)
{
  return reinterpret_cast<void*> (reinterpret_cast<physical_address_t> (address) & ~(radix - 1));
}

inline void*
align_up (void* address,
	  unsigned int radix)
{
  return reinterpret_cast<void*> ((reinterpret_cast<physical_address_t> (address) + radix - 1) & ~(radix - 1));
}

inline bool
is_aligned (const void* address,
	    unsigned int radix)
{
  return (reinterpret_cast<physical_address_t> (address) & (radix - 1)) == 0;
}

#endif /* __vm_def_hpp__ */
