/*
  File
  ----
  memory.c
  
  Description
  -----------
  Functions for controlling the physical memory including segmentation, paging, etc.

  Authors
  -------
  http://wiki.osdev.org/Higher_Half_With_GDT
  Justin R. Wilson
*/

#include "memory.h"

/* Paging constants. */
#define PAGE_SIZE 0x1000
#define PAGE_USER_MODE (1 << 2)
#define PAGE_KERNEL_MODE 0
#define PAGE_WRITABLE (1 << 1)
#define PAGE_READ_ONLY 0
#define PAGE_PRESENT (1 << 0)
#define PAGE_NOT_PRESENT
  
static unsigned int kernel_page_directory[1024] __attribute__ ((aligned (PAGE_SIZE)));
static unsigned int low_page_table[1024] __attribute__ ((aligned (PAGE_SIZE)));

void
initialize_paging ()
{
  /* Physical address of kernel_page_directory and low_page_table.
     The 0x40000000 comes from the GDT trick in loader.s
   */
  void* kernel_page_directory_paddr = (char *)kernel_page_directory + 0x40000000;
  void* low_page_table_paddr = (char *)low_page_table + 0x40000000;
  int k;

  /* Clear the kernel page directory. */
  for (k = 0; k < 1024; ++k) {
    kernel_page_directory[k] = 0;
  }

  /* Map the physical addresses 0:0x3FFFFF (4M). */
  for (k = 0; k < 1024; ++k) {
    low_page_table[k] = (k * PAGE_SIZE) | (PAGE_KERNEL_MODE | PAGE_WRITABLE | PAGE_PRESENT);
  }

  /* Low memory is mapped to the logical address ranges 0:0x3FFFFF and 0xC0000000:0xC03FFFFF. */
  kernel_page_directory[0] = (unsigned int)low_page_table_paddr | (PAGE_KERNEL_MODE | PAGE_WRITABLE | PAGE_PRESENT);
  kernel_page_directory[768] = (unsigned int)low_page_table_paddr | (PAGE_KERNEL_MODE | PAGE_WRITABLE | PAGE_PRESENT);

  /* Enable paging. */
  __asm__ __volatile__ ("mov %0, %%eax\n"
			"mov %%eax, %%cr3\n"
			"mov %%cr0, %%eax\n"
			"orl $0x80000000, %%eax\n"
			"mov %%eax, %%cr0\n" :: "m" (kernel_page_directory_paddr));
}

/* Macros for the access byte of descriptors. */

/* Is the segment present? */
#define DESC_ACCESS_NOT_PRESENT (0 << 7)
#define DESC_ACCESS_PRESENT (1 << 7)

/* What is the segment's privilege level? */
#define DESC_ACCESS_RING0 (0 << 5)
#define DESC_ACCESS_RING1 (1 << 5)
#define DESC_ACCESS_RING2 (2 << 5)
#define DESC_ACCESS_RING3 (3 << 5)

/* Does the segment describe memory or something else in the system? */
#define DESC_ACCESS_SYSTEM (0 << 4)
#define DESC_ACCESS_MEMORY (1 << 4)

/* Is the segment data or code? */
#define DESC_ACCESS_DATA (0 << 3)
#define DESC_ACCESS_CODE (1 << 3)

/* Does a data segment expand up or down ? */
#define DESC_ACCESS_EXPAND_UP (0 << 2)
#define DESC_ACCESS_EXPAND_DOWN (1 << 2)

/* Does a code segment conform (i.e., can only execute code in segments with the same privilege level)? */
#define DESC_ACCESS_NOT_CONFORMING (0 << 2)
#define DESC_ACCESS_CONFORMING (1 << 2)

/* Is a data segment writable or a code segment readable? */
#define DESC_ACCESS_DATA_WRITABLE (1 << 1)
#define DESC_ACCESS_CODE_READABLE (1 << 1)

/* Macros for the granularity byte of descriptors. */

/* Is the granularity in terms of bytes or kilobytes? */
#define DESC_GRANULARITY_BYTE (0 << 7)
#define DESC_GRANULARITY_KILOBYTE (1 << 7)

/* 16-bit or 32-bit operands? */
#define DESC_GRANULARITY_OP16 (0 << 6)
#define DESC_GRANULARITY_OP32 (1 << 6)

struct gdt_entry
{
  unsigned short limit_low;
  unsigned short base_low;
  unsigned char base_middle;
  unsigned char access;
  unsigned char granularity;
  unsigned char base_high;
} __attribute__((packed));
typedef struct gdt_entry gdt_entry_t;

struct gdt_ptr
{
  unsigned short limit;
  gdt_entry_t* base;
} __attribute__((packed));
typedef struct gdt_ptr gdt_ptr_t;

#define DESCRIPTOR_COUNT 3

static gdt_entry_t gdt[DESCRIPTOR_COUNT];
static gdt_ptr_t gp;

extern void
gdt_flush (gdt_ptr_t*);

static void
gdt_set_gate (unsigned int descriptor_offset,
	      unsigned int base,
	      unsigned int limit,
	      unsigned char access,
	      unsigned char granularity)
{
  unsigned int num = descriptor_offset / sizeof (gdt_entry_t);
  gdt[num].base_low = (base & 0xFFFF);
  gdt[num].base_middle = (base >> 16) & 0xFF;
  gdt[num].base_high = (base >> 24) & 0xFF;

  gdt[num].limit_low = (limit & 0xFFFF);
  gdt[num].granularity = (granularity & 0xF0) | ((limit >> 16) & 0x0F);
  gdt[num].access = access;
}

void
install_gdt ()
{
  gp.limit = (sizeof (gdt_entry_t) * DESCRIPTOR_COUNT) - 1;
  gp.base = gdt;

  gdt_set_gate (0, 0, 0, 0, 0);
  gdt_set_gate (KERNEL_CODE_SEGMENT, 0, 0xFFFFFFFF, (DESC_ACCESS_PRESENT | DESC_ACCESS_RING0 | DESC_ACCESS_MEMORY | DESC_ACCESS_CODE | DESC_ACCESS_NOT_CONFORMING | DESC_ACCESS_CODE_READABLE), (DESC_GRANULARITY_KILOBYTE | DESC_GRANULARITY_OP32));
  gdt_set_gate (KERNEL_DATA_SEGMENT, 0, 0xFFFFFFFF, (DESC_ACCESS_PRESENT | DESC_ACCESS_RING0 | DESC_ACCESS_MEMORY | DESC_ACCESS_DATA | DESC_ACCESS_EXPAND_UP | DESC_ACCESS_DATA_WRITABLE), (DESC_GRANULARITY_KILOBYTE | DESC_GRANULARITY_OP32));

  gdt_flush (&gp);
}
