#ifndef __vm_def_hpp__
#define __vm_def_hpp__

#include <stdint.h>
#include <stddef.h>

typedef uint32_t physical_address_t;
typedef uint32_t frame_t;

typedef uint_fast16_t page_table_idx_t;

static const size_t PAGE_SIZE = 0x1000;

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

// One megabyte is important for dealing with low memory.
static const size_t ONE_MEGABYTE = 0x00100000;

// Four megabytes are important because that is span of one page table or page directory entry.
static const size_t FOUR_MEGABYTES = 0x00400000;

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

typedef uint32_t page_fault_error_t;

inline bool
not_present (page_fault_error_t error)
{
  return (error & (1 << 0)) == 0;
}

inline bool
protection_violation (page_fault_error_t error)
{
  return (error & (1 << 0)) != 0;
}

inline bool
read_context (page_fault_error_t error)
{
  return (error & (1 << 1)) == 0;
}

inline bool
write_context (page_fault_error_t error)
{
  return (error & (1 << 1)) != 0;
}

inline bool
supervisor_context (page_fault_error_t error)
{
  return (error & (1 << 2)) == 0;
}

inline bool
user_context (page_fault_error_t error)
{
  return (error & (1 << 2)) != 0;
}

inline bool
reserved_bit_violation (page_fault_error_t error)
{
  return (error & (1 << 3)) != 0;
}

inline bool
data_context (page_fault_error_t error)
{
  return (error & (1 << 4)) == 0;
}

inline bool
instruction_context (page_fault_error_t error)
{
  return (error & (1 << 4)) != 0;
}

#endif /* __vm_def_hpp__ */
