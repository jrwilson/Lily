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
#include "interrupt.h"
#include "kput.h"
#include "halt.h"

/* Paging constants. */
#define PAGE_SIZE 0x1000
#define PAGE_USER_MODE (1 << 2)
#define PAGE_KERNEL_MODE 0
#define PAGE_WRITABLE (1 << 1)
#define PAGE_READ_ONLY 0
#define PAGE_PRESENT (1 << 0)
#define PAGE_NOT_PRESENT

#define PAGE_ENTRY_COUNT 1024

#define PAGE_DIRECTORY_ENTRY(addr)  ((addr & 0xFFC00000) >> 22)
#define PAGE_TABLE_ENTRY(addr) ((addr & 0x3FF000) >> 12)
#define PAGE_ALIGN(addr) (addr & 0xFFFFF000)

extern unsigned int end_of_kernel;  
static unsigned int identity_map_limit;
static unsigned int kernel_page_directory[PAGE_ENTRY_COUNT] __attribute__ ((aligned (PAGE_SIZE)));
static unsigned int low_page_table[PAGE_ENTRY_COUNT] __attribute__ ((aligned (PAGE_SIZE)));

void
initialize_paging ()
{
  /* Physical address of kernel_page_directory and low_page_table.
     The 0x40000000 comes from the GDT trick in loader.s.
     The 0xC0000000 comes fromt he higher-half placement.
   */
  void* kernel_page_directory_paddr = (char *)kernel_page_directory + 0x40000000;
  void* low_page_table_paddr = (char *)low_page_table + 0x40000000;
  unsigned int k;

  /* &end_of_kernel is the first address in the page after the kernel.
     We adust the value due to the higher-half placement (0xC0000000).
     We will identity map up to this address. */
  identity_map_limit = ((unsigned int)&end_of_kernel) - 0xC0000000;

  /* Clear the kernel page directory and low page table. */
  for (k = 0; k < PAGE_ENTRY_COUNT; ++k) {
    kernel_page_directory[k] = 0;
    low_page_table[k] = 0;
  }

  /* Identity map the physical addresses from 0 to the end of the kernel. */
  for (k = 0; k < identity_map_limit; k += PAGE_SIZE) {
    low_page_table[PAGE_TABLE_ENTRY (k)] = k | (PAGE_KERNEL_MODE | PAGE_WRITABLE | PAGE_PRESENT);
  }

  /* Low memory is mapped to the logical address ranges 0:0x3FFFFF and 0xC0000000:0xC03FFFFF. */
  kernel_page_directory[0] = (unsigned int)low_page_table_paddr | (PAGE_KERNEL_MODE | PAGE_WRITABLE | PAGE_PRESENT);
  kernel_page_directory[768] = (unsigned int)low_page_table_paddr | (PAGE_KERNEL_MODE | PAGE_WRITABLE | PAGE_PRESENT);

  /* Enable paging. */
  __asm__ __volatile__ ("mov %0, %%eax\n"
			"mov %%eax, %%cr3\n"
			"mov %%cr0, %%eax\n"
			"orl $0x80000000, %%eax\n"
			"mov %%eax, %%cr0\n" :: "m" (kernel_page_directory_paddr) : "%eax");
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

void
identity_map_up_to (unsigned int addr)
{
  addr = PAGE_ALIGN (addr) + PAGE_SIZE;
  for (; identity_map_limit < addr; identity_map_limit += PAGE_SIZE) {
    /* Must be in the low page table (first 4M of physical memory). */
    if (PAGE_DIRECTORY_ENTRY (identity_map_limit) == 0) {
      low_page_table[PAGE_TABLE_ENTRY (identity_map_limit)] = identity_map_limit | (PAGE_KERNEL_MODE | PAGE_WRITABLE | PAGE_PRESENT);
      /* Invalidate entry in TLB. */
      __asm__ __volatile__ ("invlpg %0\n" :: "m" (identity_map_limit));
    }
  }
}

#define PAGE_FAULT_INTERRUPT 14

#define PAGE_PROTECTION_ERROR (1 << 0)
#define PAGE_WRITE_ERROR (1 << 1)
#define PAGE_USER_ERROR (1 << 2)
#define PAGE_RESERVED_ERROR (1 << 3)
#define PAGE_INSTRUCTION_ERROR (1 << 4)

static void
page_fault_handler (registers_t* regs)
{
  kputs ("Page fault!!\n");
  unsigned int addr;
  __asm__ __volatile__ ("mov %%cr2, %0\n" : "=r"(addr) :: );

  kputs ("Address: "); kputuix (addr); kputs ("\n");
  kputs ("Codes: ");
  if (regs->error & PAGE_PROTECTION_ERROR) {
    kputs ("PROTECTION ");
  }
  else {
    kputs ("NOT_PRESENT ");
  }
  if (regs->error & PAGE_WRITE_ERROR) {
    kputs ("WRITE ");
  }
  else {
    kputs ("READ ");
  }
  if (regs->error & PAGE_USER_ERROR) {
    kputs ("USER ");
  }
  else {
    kputs ("SUPERVISOR ");
  }
  if (regs->error & PAGE_RESERVED_ERROR) {
    kputs ("RESERVED ");
  }
  if (regs->error & PAGE_INSTRUCTION_ERROR) {
    kputs ("INSTRUCTION ");
  }
  else {
    kputs ("DATA ");
  }
  kputs ("\n");

  kputs ("CS: "); kputuix (regs->cs); kputs (" EIP: "); kputuix (regs->eip); kputs (" EFLAGS: "); kputuix (regs->eflags); kputs ("\n");
  kputs ("SS: "); kputuix (regs->ss); kputs (" ESP: "); kputuix (regs->useresp); kputs (" DS:"); kputuix (regs->ds); kputs ("\n");
  
  kputs ("EAX: "); kputuix (regs->eax); kputs (" EBX: "); kputuix (regs->ebx); kputs (" ECX: "); kputuix (regs->ecx); kputs (" EDX: "); kputuix (regs->edx); kputs ("\n");
  kputs ("ESP: "); kputuix (regs->esp); kputs (" EBP: "); kputuix (regs->ebp); kputs (" ESI: "); kputuix (regs->esi); kputs (" EDI: "); kputuix (regs->edi); kputs ("\n");

  kputs ("Halting");
  halt ();
}

void
install_page_fault_handler ()
{
  set_interrupt_handler (PAGE_FAULT_INTERRUPT, page_fault_handler);
}
