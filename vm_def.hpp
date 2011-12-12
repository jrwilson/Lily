#ifndef __vm_def_hpp__
#define __vm_def_hpp__

#include <stdint.h>

typedef uint32_t physical_address_t;
typedef uint32_t frame_t;

typedef uint_fast16_t page_table_idx_t;

#define PAGE_SIZE 0x1000

/* Number of entries in a page table or directory. */
static const page_table_idx_t PAGE_ENTRY_COUNT = 1024;

static const int FRAME_SHIFT = 12;

/* Don't mess with memory below 1M or above 4G. */
static const physical_address_t USABLE_MEMORY_BEGIN = 0x00100000;
static const physical_address_t USABLE_MEMORY_END = 0xFFFFF000;

/* Should agree with loader.s. */
static const void* const KERNEL_VIRTUAL_BASE = reinterpret_cast<const void*> (0xC0000000);

/* Everything must fit into 4MB initialy. */
static const void* const INITIAL_LOGICAL_LIMIT = reinterpret_cast<const void*> (0xC0400000);

/* Memory under one megabyte is not used. */
static const unsigned int ONE_MEGABYTE = 0x00100000;

/* Logical address above this address are using for page tables. */
static const void* const PAGING_AREA = reinterpret_cast<const void*> (0xFFC00000);

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

inline physical_address_t
logical_to_physical (const void* address,
		     const void* offset)
{
  return reinterpret_cast<physical_address_t> (address) - reinterpret_cast<physical_address_t> (offset);
}

inline page_table_idx_t
get_page_table_entry (const void* address)
{
  return (reinterpret_cast<page_table_idx_t> (address) & 0x3FF000) >> 12;
}

inline page_table_idx_t
get_page_directory_entry (const void* address)
{
  return (reinterpret_cast<page_table_idx_t> (address) & 0xFFC00000) >> 22;
}

inline const void*
get_address (page_table_idx_t directory_entry,
	     page_table_idx_t table_entry)
{
  return reinterpret_cast<const void*> (directory_entry << 22 | table_entry << 12);
}

#endif /* __vm_def_hpp__ */
