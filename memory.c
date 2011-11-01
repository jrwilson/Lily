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
#include "kassert.h"
#include "descriptor.h"

/* Page is 4 kilobytes. */
#define PAGE_SIZE 0x1000

/* Number of entries in a page table or directory. */
#define PAGE_ENTRY_COUNT 1024

/* Macros for address manipulation. */
#define PAGE_DIRECTORY_ENTRY(addr)  (((addr) & 0xFFC00000) >> 22)
#define PAGE_TABLE_ENTRY(addr) (((addr) & 0x3FF000) >> 12)
#define PAGE_ALIGN(addr) ((addr) & 0xFFFFF000)
#define PAGE_ADDRESS_OFFSET(addr) ((addr) & 0xFFF)
#define IS_PAGE_ALIGNED(addr) (((addr) & 0xFFF) == 0)

/* Macros for page faults. */
#define PAGE_FAULT_INTERRUPT 14

#define PAGE_PROTECTION_ERROR (1 << 0)
#define PAGE_WRITE_ERROR (1 << 1)
#define PAGE_USER_ERROR (1 << 2)
#define PAGE_RESERVED_ERROR (1 << 3)
#define PAGE_INSTRUCTION_ERROR (1 << 4)

/* Can identity map up to 4 megabytes. */
#define IDENTITY_LIMIT 0x400000

/* Convert physical addresses to frame numbers and vice versa. */
#define ADDRESS_TO_FRAME(addr) ((addr) >> 12)
#define FRAME_TO_ADDRESS(addr) ((addr) << 12)

/* Convert frame numbers to indexes and offsets in a bitset. */
#define FRAME_TO_INDEX(frame) ((frame) >> 5)
#define FRAME_TO_OFFSET(frame) ((frame) & 0x1F)
#define INDEX_OFFSET_TO_FRAME(idx, off) (((idx) << 5) | (off))

/* Don't mess with memory below 1M. */
#define MEMORY_LOWER_LIMIT 0x100000

#define HEAP_INITIAL_SIZE 0x8000
#define HEAP_MAGIC 0x7ACEDEAD

#define USER_STACK_INITIAL_SIZE PAGE_SIZE

struct page_table_entry {
  unsigned int present : 1;
  unsigned int writable : 1;
  unsigned int user : 1;
  unsigned int write_through : 1;
  unsigned int cache_disabled : 1;
  unsigned int accessed : 1;
  unsigned int dirty : 1;
  unsigned int zero : 1;
  unsigned int global : 1;
  unsigned int available : 3;
  unsigned int frame : 20;
};
typedef struct page_table_entry page_table_entry_t;

struct page_table {
  page_table_entry_t entries[PAGE_ENTRY_COUNT];
};
typedef struct page_table page_table_t;

struct page_directory_entry {
  unsigned int present : 1;
  unsigned int writable : 1;
  unsigned int user : 1;
  unsigned int write_through : 1;
  unsigned int cache_disabled : 1;
  unsigned int accessed : 1;
  unsigned int zero : 1;
  unsigned int page_size : 1;
  unsigned int ignored : 1;
  unsigned int available : 3;
  unsigned int frame : 20;
};
typedef struct page_directory_entry page_directory_entry_t;

struct page_directory {
  /* Physical addresses.  This is first so we can page-align it. */
  page_directory_entry_t entries_physical[PAGE_ENTRY_COUNT];
  page_table_t* entries_logical[PAGE_ENTRY_COUNT];
  unsigned int physical_address;
  page_directory_t* next;
};

typedef enum {
  NOT_GLOBAL = 0,
  GLOBAL = 1,
} global_t;

typedef enum {
  CACHED = 0,
  NOT_CACHED = 1,
} cached_t;

typedef enum {
  WRITE_BACK = 0,
  WRITE_THROUGH = 1,
} cache_mode_t;

typedef enum {
  SUPERVISOR = 0,
  USER = 1,
} page_privilege_t;

typedef enum {
  PAGE_SIZE_4K = 0,
  PAGE_SIZE_4M = 1,
} page_size_t;

/* The heap progresses through three modes.

   The IDENTITY mode extends the mapping of the low memory.
   The main purpose of this mode is to read GRUB data structures.
   Memory allocated in IDENTITY mode will have a logical address under 4M.
   Memory allocated in IDENTITY mode can be reclaimed.

   The PLACEMENT mode extends the mapping of the low memory.
   The main purpose of this mode is to prepare for HEAP mode.
   While the low memory map is extended, memory allocated in PLACEMENT mode will have a logical address above KERNEL_OFFSET.
   This allows us to unmap the memory below 1M.
   Memory allocated in PLACEMENT mode can not be reclaimed.

   The HEAP mode represents a fully functional dynamic memory system.
   Memory allocated in HEAP mode will have a logical address above KERNEL_OFFSET.
   Memory allocated in HEAP mode can be reclaimed.
 */
enum heap_mode {
  IDENTITY,
  PLACEMENT,
  HEAP
};
typedef enum heap_mode heap_mode_t;

struct gdt_ptr
{
  unsigned short limit;
  descriptor_t* base;
} __attribute__((packed));
typedef struct gdt_ptr gdt_ptr_t;

typedef struct frame_set_item frame_set_item_t;
struct frame_set_item {
  frame_set_item_t* next;
  unsigned int begin;
  unsigned int end;
  unsigned int free_count;
  unsigned int next_free;
  unsigned int bitset_size;
  unsigned int bitset[];
};

typedef struct header header_t;
struct header {
  unsigned int available : 1;
  unsigned int magic : 31;
  unsigned int size; /* Does not include header. */
  header_t* prev;
  header_t* next;
};

extern void
gdt_flush (gdt_ptr_t*);
	       
/* Take the address of this symbol to find the end of the kernel rounded up to the next page. */
extern unsigned int end_of_kernel_page_aligned; 

static heap_mode_t heap_mode;
/* The set [placement_base - KERNEL_OFFSET, placement_limit - KERNEL_OFFSET) is a subset of [identity_base, identity_limit). */
static unsigned int identity_base;
static unsigned int identity_limit;
static unsigned int placement_base;
static unsigned int placement_limit;
static unsigned int heap_base;
static unsigned int heap_limit;
static unsigned int user_stack_base;

static page_directory_t kernel_page_directory_var __attribute__ ((aligned (PAGE_SIZE)));
static page_table_t low_page_table __attribute__ ((aligned (PAGE_SIZE)));

page_directory_t* kernel_page_directory = &kernel_page_directory_var;
static page_directory_t* page_directories = &kernel_page_directory_var;
static page_directory_t* current_page_directory = 0;
static page_table_t* spare_page_table = 0;

static descriptor_t gdt[DESCRIPTOR_COUNT];
static gdt_ptr_t gp;

static frame_set_item_t* frame_set = 0;
static unsigned int total_frames = 0;
static unsigned int used_frames = 0;

static header_t* heap_first;
static header_t* heap_last;

static void
memset (void* ptr,
	unsigned char value,
	unsigned int size)
{
  unsigned char* c = ptr;
  while (size != 0) {
    *c = value;
    ++c;
    --size;
  }
}

static unsigned int
logical_to_physical (page_directory_t* ptr,
		     unsigned int logical_addr)
{
  kassert (ptr != 0);

  page_table_t* pt = ptr->entries_logical[PAGE_DIRECTORY_ENTRY (logical_addr)];

  if (pt != 0) {
    return FRAME_TO_ADDRESS (pt->entries[PAGE_TABLE_ENTRY (logical_addr)].frame) | PAGE_ADDRESS_OFFSET (logical_addr);
  }
  else {
    return 0;
  }
}

static page_table_entry_t
make_page_table_entry (unsigned int frame,
		       global_t global,
		       cached_t cached,
		       cache_mode_t cache_mode,
		       page_privilege_t privilege,
		       writable_t writable,
		       present_t present)
{
  page_table_entry_t retval;
  retval.frame = frame;
  retval.available = 0;
  retval.global = global;
  retval.zero = 0;
  retval.dirty = 0;
  retval.accessed = 0;
  retval.cache_disabled = cached;
  retval.write_through = cache_mode;
  retval.user = privilege;
  retval.writable = writable;
  retval.present = present;
  return retval;
}

static page_directory_entry_t
make_page_directory_entry (unsigned int frame,
			   page_size_t page_size,
			   cached_t cached,
			   cache_mode_t cache_mode,
			   page_privilege_t privilege,
			   writable_t writable,
			   present_t present)
{
  page_directory_entry_t retval;
  retval.frame = frame;
  retval.available = 0;
  retval.ignored = 0;
  retval.page_size = page_size;
  retval.zero = 0;
  retval.accessed = 0;
  retval.cache_disabled = cached;
  retval.write_through = cache_mode;
  retval.user = privilege;
  retval.writable = writable;
  retval.present = present;
  return retval;
}

static void
initialize_page_table (page_table_t* ptr)
{
  kassert (ptr != 0);
  kassert (IS_PAGE_ALIGNED ((unsigned int)ptr));
  memset (ptr, 0, sizeof (page_table_t));
}

static page_table_t*
allocate_page_table ()
{
  page_table_t* ptr = kmalloc_pa (sizeof (page_table_t));
  initialize_page_table (ptr);
  return ptr;
}

static void
initialize_page_directory (page_directory_t* ptr,
			   unsigned int physical_address)
{
  kassert (ptr != 0);
  kassert (IS_PAGE_ALIGNED ((unsigned int)ptr));
  kassert (IS_PAGE_ALIGNED ((unsigned int)physical_address));

  memset (ptr, 0, sizeof (page_directory_t));
  ptr->physical_address = physical_address;
}

page_directory_t*
allocate_page_directory ()
{
  page_directory_t* ptr = kmalloc_pa (sizeof (page_directory_t));
  /* Copy from the kernel. */
  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT; ++k) {
    ptr->entries_physical[k] = kernel_page_directory->entries_physical[k];
    ptr->entries_logical[k] = kernel_page_directory->entries_logical[k];
  }
  ptr->physical_address = logical_to_physical (kernel_page_directory, (unsigned int)ptr);
  /* Add to the list. */
  ptr->next = page_directories;
  page_directories = ptr;

  return ptr;
}

void
switch_to_page_directory (page_directory_t* ptr)
{
  kassert (ptr != 0);
  kassert (IS_PAGE_ALIGNED ((unsigned int)ptr));

  /* Switch to the page directory. */
  current_page_directory = ptr;
  asm volatile ("mov %0, %%eax\n"
		"mov %%eax, %%cr3\n" :: "m" (ptr->physical_address) : "%eax");
}

static void
insert_page_table (page_directory_t* ptr,
		   unsigned int logical_addr,
		   page_directory_entry_t page_directory_entry,
		   page_table_t* pt)
{
  kassert (ptr != 0);
  kassert (IS_PAGE_ALIGNED ((unsigned int)ptr));
  kassert (IS_PAGE_ALIGNED (logical_addr));
  kassert (ptr->entries_logical[PAGE_DIRECTORY_ENTRY (logical_addr)] == 0);
  kassert (pt != 0);
  
  ptr->entries_physical[PAGE_DIRECTORY_ENTRY (logical_addr)] = page_directory_entry;
  ptr->entries_logical[PAGE_DIRECTORY_ENTRY (logical_addr)] = pt;

  if (ptr == kernel_page_directory) {
    /* Map into all page directories. */
    page_directory_t* p;
    for (p = page_directories; p != 0; p = p->next) {
      p->entries_physical[PAGE_DIRECTORY_ENTRY (logical_addr)] = page_directory_entry;
      p->entries_logical[PAGE_DIRECTORY_ENTRY (logical_addr)] = pt;
    }
  }
}

static void
insert_mapping (page_directory_t* ptr,
		unsigned int logical_addr,
		page_table_entry_t page_table_entry,
		page_directory_entry_t page_directory_entry)
{
  kassert (ptr != 0);
  kassert (IS_PAGE_ALIGNED ((unsigned int)ptr));
  kassert (IS_PAGE_ALIGNED (logical_addr));

  if (ptr->entries_logical[PAGE_DIRECTORY_ENTRY (logical_addr)] == 0) {
    /* We need a new page table.
       We have pre-allocated one for this purpose. */
    kassert (spare_page_table != 0);
    page_directory_entry.frame = ADDRESS_TO_FRAME (logical_to_physical (ptr, (unsigned int)spare_page_table));
    insert_page_table (ptr, logical_addr, page_directory_entry, spare_page_table);
    spare_page_table = 0;
  }

  ptr->entries_logical[PAGE_DIRECTORY_ENTRY (logical_addr)]->entries[PAGE_TABLE_ENTRY (logical_addr)] = page_table_entry;
  if (ptr == current_page_directory) {
    /* Invalidate entry in TLB. */
    asm volatile ("invlpg %0\n" :: "m" (logical_addr));
  }
}

void
initialize_paging ()
{
  heap_mode = IDENTITY;
  identity_base = ((unsigned int)&end_of_kernel_page_aligned) - KERNEL_OFFSET;
  identity_limit = identity_base;
  kassert (identity_limit <= IDENTITY_LIMIT);

  unsigned int k;

  /* To compute the physical address of the page directory, we add 0x40000000 according to the GDT trick in loader.s. */
  initialize_page_directory (kernel_page_directory, (unsigned int)kernel_page_directory + 0x40000000);
  initialize_page_table (&low_page_table);

 /* To compute the physical address of the page table, we add 0x40000000 according to the GDT trick in loader.s. */
  unsigned int low_page_table_paddr = (unsigned int)&low_page_table + 0x40000000;
  /* Low memory (4 megabytes) is mapped to the logical address ranges 0:0x3FFFFF and 0xC0000000:0xC03FFFFF. */
  insert_page_table (kernel_page_directory, 0, make_page_directory_entry (ADDRESS_TO_FRAME (low_page_table_paddr), PAGE_SIZE_4K, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), &low_page_table);
  insert_page_table (kernel_page_directory, KERNEL_OFFSET, make_page_directory_entry (ADDRESS_TO_FRAME (low_page_table_paddr), PAGE_SIZE_4K, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), &low_page_table);

  /* Identity map the physical addresses from 0 to the end of the kernel. */
  for (k = 0; k < identity_limit; k += PAGE_SIZE) {
    insert_mapping (kernel_page_directory, k, make_page_table_entry (ADDRESS_TO_FRAME (k), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), make_page_directory_entry (0, PAGE_SIZE_4K, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT));
  }
 
  /* Switch to the page directory. */
  switch_to_page_directory (kernel_page_directory);

  /* Enable paging. */
  asm volatile ("mov %%cr0, %%eax\n"
		"orl $0x80000000, %%eax\n"
		"mov %%eax, %%cr0\n" ::: "%eax");
}

/* struct tss { */
/*   unsigned int prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list. */
/*   unsigned int esp0;       // The stack pointer to load when we change to kernel mode. */
/*   unsigned int ss0;        // The stack segment to load when we change to kernel mode. */
/*   unsigned int esp1;       // Unused... */
/*   unsigned int ss1; */
/*   unsigned int esp2; */
/*   unsigned int ss2; */
/*   unsigned int cr3; */
/*   unsigned int eip; */
/*   unsigned int eflags; */
/*   unsigned int eax; */
/*   unsigned int ecx; */
/*   unsigned int edx; */
/*   unsigned int ebx; */
/*   unsigned int esp; */
/*   unsigned int ebp; */
/*   unsigned int esi; */
/*   unsigned int edi; */
/*   unsigned int es;         // The value to load into ES when we change to kernel mode. */
/*   unsigned int cs;         // The value to load into CS when we change to kernel mode. */
/*   unsigned int ss;         // The value to load into SS when we change to kernel mode. */
/*   unsigned int ds;         // The value to load into DS when we change to kernel mode. */
/*   unsigned int fs;         // The value to load into FS when we change to kernel mode. */
/*   unsigned int gs;         // The value to load into GS when we change to kernel mode. */
/*   unsigned int ldt;        // Unused... */
/*   unsigned short trap; */
/*   unsigned short iomap_base; */
/* }; */
/* typedef struct tss tss_t; */

/* tss_t tss; */

void
install_gdt ()
{
  gp.limit = (sizeof (descriptor_t) * DESCRIPTOR_COUNT) - 1;
  gp.base = gdt;

  gdt[0] = make_null_descriptor ();
  gdt[KERNEL_CODE_SELECTOR / sizeof (descriptor_t)].code_segment = make_code_segment_descriptor (0, 0xFFFFFFFF, READABLE, NOT_CONFORMING, RING0, PRESENT, WIDTH_32, PAGE_GRANULARITY); 
  gdt[KERNEL_DATA_SELECTOR / sizeof (descriptor_t)].data_segment = make_data_segment_descriptor (0, 0xFFFFFFFF, WRITABLE, EXPAND_UP, RING0, PRESENT, WIDTH_32, PAGE_GRANULARITY);
  gdt[USER_CODE_SELECTOR / sizeof (descriptor_t)].code_segment = make_code_segment_descriptor (0, 0xFFFFFFFF, READABLE, NOT_CONFORMING, RING3, PRESENT, WIDTH_32, PAGE_GRANULARITY);
  gdt[USER_DATA_SELECTOR / sizeof (descriptor_t)].data_segment = make_data_segment_descriptor (0, 0xFFFFFFFF, WRITABLE, EXPAND_UP, RING3, PRESENT, WIDTH_32, PAGE_GRANULARITY);

  /* memset (&tss, 0, sizeof (tss_t)); */
  /* tss.ss0 = KERNEL_DATA_SEGMENT; */
  /* tss.esp0 = 0xDEADBEEF; /\* This needs to be fixed. *\/ */
  /* tss.cs = KERNEL_CODE_SEGMENT | RING3; */
  /* tss.ss = KERNEL_DATA_SEGMENT | RING3; */
  /* tss.ds = KERNEL_DATA_SEGMENT | RING3; */
  /* tss.es = KERNEL_DATA_SEGMENT | RING3; */
  /* tss.fs = KERNEL_DATA_SEGMENT | RING3; */
  /* tss.gs = KERNEL_DATA_SEGMENT | RING3; */

  /* initialize_tss_descriptor (&gdt[TSS_SELECTOR / sizeof (descriptor_t)].tss, (unsigned int)&tss, (unsigned int)&tss + sizeof (tss_t) - 1, RING3, PRESENT, BYTE_GRANULARITY); */

  gdt_flush (&gp);

  /* unsigned int sel = TSS_SELECTOR | RING3; */
  /* asm volatile ("mov %0, %%eax\n" */
  /* "ltr %%ax\n" : : "m"(sel) ); */
}

void
extend_identity (unsigned int addr)
{
  kassert (heap_mode == IDENTITY || heap_mode == PLACEMENT);
  kassert (addr < IDENTITY_LIMIT);

  addr = PAGE_ALIGN (addr + PAGE_SIZE - 1);
  for (; identity_limit < addr; identity_limit += PAGE_SIZE) {
    insert_mapping (kernel_page_directory, identity_limit, make_page_table_entry (ADDRESS_TO_FRAME (identity_limit), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), make_page_directory_entry (0, PAGE_SIZE_4K, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT));
  }
}

static void
page_fault_handler (registers_t* regs)
{
  kputs ("Page fault!!\n");
  unsigned int addr;
  asm volatile ("mov %%cr2, %0\n" : "=r"(addr));

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

static void*
placement_alloc (unsigned int size)
{
  kassert (heap_mode == PLACEMENT);

  char* retval = (char*)placement_limit;
  placement_limit += size;
  extend_identity (placement_limit - KERNEL_OFFSET - 1);
  return retval;
}

static frame_set_item_t*
allocate_frame_set_item (unsigned int base,
			 unsigned int size)
{
  unsigned int frames = ADDRESS_TO_FRAME (size);
  total_frames += frames;
  /* Because we can store 2^5 = 32 frames per entry. */
  unsigned int bitset_size = FRAME_TO_INDEX (frames + 31);
  frame_set_item_t* retval = placement_alloc (sizeof (frame_set_item_t) + bitset_size * sizeof (unsigned int));
  kassert (retval != 0);
  retval->begin = ADDRESS_TO_FRAME (base);
  retval->end = retval->begin + frames;
  retval->free_count = frames;
  retval->next_free = 0;
  retval->bitset_size = bitset_size;
  memset (retval->bitset, 0, bitset_size * sizeof (unsigned int));
  /* Set any left over bits. */
  for (; frames < bitset_size * 32; ++frames) {
    retval->bitset[FRAME_TO_INDEX (frames)] |= (1 << FRAME_TO_OFFSET (frames));
  }

  return retval;
}

static void
frame_set_item_set (frame_set_item_t* ptr,
		    unsigned int frame)
{
  kassert (ptr != 0);
  kassert (frame >= ptr->begin);
  kassert (frame < ptr->end);

  /* Normalize. */
  frame -= ptr->begin;
  /* Should not be set. */
  kassert ((ptr->bitset[FRAME_TO_INDEX (frame)] & (1 << FRAME_TO_OFFSET (frame))) == 0);
  /* Set. */
  ptr->bitset[FRAME_TO_INDEX (frame)] |= (1 << FRAME_TO_OFFSET (frame));
  --ptr->free_count;
  ++used_frames;
  if (ptr->bitset[FRAME_TO_INDEX (frame)] != 0xFFFFFFFF) {
    ptr->next_free = FRAME_TO_INDEX (frame);
  }
}

static unsigned int
frame_set_item_allocate (frame_set_item_t* ptr)
{
  kassert (ptr != 0);
  kassert (ptr->free_count != 0);

  unsigned int i;
  for (i = 0; i < ptr->bitset_size; ++i) {
    if (ptr->bitset[ptr->next_free] != 0xFFFFFFFF) {
      /* ptr->next_free has a frame. */
      unsigned int offset;
      for (offset = 0; offset < 32; ++offset) {
	if ((ptr->bitset[ptr->next_free] & (1 << offset)) == 0) {
	  /* Set. */
	  ptr->bitset[ptr->next_free] |= (1 << offset);
	  --ptr->free_count;
	  ++used_frames;
	  return ptr->begin + INDEX_OFFSET_TO_FRAME (ptr->next_free, offset);
	}
      }
    }
    ptr->next_free = (ptr->next_free + 1) % ptr->bitset_size;
  }

  /* Should find one. */
  kassert (0);
  return 0;
}

static void
frame_manager_set (unsigned int frame)
{
  /* Find the item corresponding to frame. */
  frame_set_item_t* ptr;
  for (ptr = frame_set; ptr != 0 && !(frame >= ptr->begin && frame < ptr->end); ptr = ptr->next) ;;
  if (ptr != 0) {
    /* Set the appropriate bit. */
    frame_set_item_set (ptr, frame);
  }
}

static unsigned int
frame_manager_allocate ()
{
  frame_set_item_t* ptr;
  for (ptr = frame_set; ptr != 0 && ptr->free_count == 0; ptr = ptr->next) ;;
  if (ptr != 0) {
    return frame_set_item_allocate (ptr);
  }

  /* Out of memory. */
  kassert (0);
  return 0;
}

void
initialize_heap (multiboot_info_t* mbd)
{
  kassert (heap_mode == IDENTITY);

  placement_base = identity_limit + KERNEL_OFFSET;
  placement_limit = placement_base;
  heap_mode = PLACEMENT;

  /* Parse the multiboot data structure. */
  multiboot_memory_map_t* ptr = (multiboot_memory_map_t*)mbd->mmap_addr;
  multiboot_memory_map_t* limit = (multiboot_memory_map_t*)(mbd->mmap_addr + mbd->mmap_length);
  while (ptr < limit) {
    kputuix (ptr->addr); kputs ("-"); kputuix (ptr->addr + ptr->len);
    switch (ptr->type) {
    case MULTIBOOT_MEMORY_AVAILABLE:
      kputs (" AVAILABLE");
      if (ptr->addr >= MEMORY_LOWER_LIMIT) {
	frame_set_item_t* p = allocate_frame_set_item (ptr->addr, ptr->len);
	p->next = frame_set;
	frame_set = p;
      }
      break;
    case MULTIBOOT_MEMORY_RESERVED:
      kputs (" RESERVED");
      break;
    default:
      kputs (" UNKNOWN");
      break;
    }
    kputs ("\n");
    /* Move to the next descriptor. */
    ptr = (multiboot_memory_map_t*) (((char*)&(ptr->addr)) + ptr->size);
  }

  /* All of the frames (physical pages) up to identity_limit are in use. */
  unsigned int frame;
  for (frame = 0; frame < ADDRESS_TO_FRAME (identity_limit); ++frame) {
    frame_manager_set (frame);
  }

  heap_base = identity_limit + KERNEL_OFFSET;
  heap_limit = heap_base;

  /* Grow the heap to its initial size. */
  for (; (heap_limit - heap_base) < HEAP_INITIAL_SIZE; heap_limit += PAGE_SIZE) {
    insert_mapping (kernel_page_directory, heap_limit, make_page_table_entry (frame_manager_allocate (), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), make_page_directory_entry (0, PAGE_SIZE_4K, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT));
  }

  /* Imprint the heap. */
  heap_first = (header_t*)heap_base;
  heap_last = heap_first;

  heap_first->available = 1;
  heap_first->magic = HEAP_MAGIC;
  heap_first->size = heap_limit - heap_base - sizeof (header_t);
  heap_first->prev = 0;
  heap_first->next = 0;

  /* Heap is now operational. */
  heap_mode = HEAP;

  /* Allocate a spare page table so we can always exand the kernel's address space. */
  spare_page_table = allocate_page_table ();

  /* Initialize the stack. */
  user_stack_base = USER_STACK_LIMIT - USER_STACK_INITIAL_SIZE;

  unsigned int stack_addr;
  /* Use != due to wrap around. */
  for (stack_addr = user_stack_base; stack_addr != USER_STACK_LIMIT; stack_addr += PAGE_SIZE) {
    insert_mapping (kernel_page_directory, stack_addr, make_page_table_entry (frame_manager_allocate (), NOT_GLOBAL, CACHED, WRITE_BACK, USER, WRITABLE, PRESENT), make_page_directory_entry (0, PAGE_SIZE_4K, CACHED, WRITE_BACK, USER, WRITABLE, PRESENT));
  }
  
  /* kputs ("Total frames: "); kputuix (total_frames); kputs ("\n"); */
  /* kputs ("Used frames: "); kputuix (used_frames); kputs ("\n"); */

  /* kputs ("identity_base: "); kputuix (identity_base); kputs ("\n"); */
  /* kputs ("identity_limit: "); kputuix (identity_limit); kputs ("\n"); */
  /* kputs ("placement_base: "); kputuix (placement_base); kputs ("\n"); */
  /* kputs ("placement_limit: "); kputuix (placement_limit); kputs ("\n"); */
  /* kputs ("heap_base: "); kputuix (heap_base); kputs ("\n"); */
  /* kputs ("heap_limit: "); kputuix (heap_limit); kputs ("\n"); */
  /* kputs ("user_stack_base: "); kputuix (user_stack_base); kputs ("\n"); */
  /* kputs ("user_stack_limit: "); kputuix (user_stack_limit); kputs ("\n"); */
}

static void
dump_heap_int (header_t* ptr)
{
  if (ptr != 0) {
    kputp (ptr); kputs (" "); kputuix (ptr->magic); kputs (" "); kputuix (ptr->size); kputs (" "); kputp (ptr->prev); kputs (" "); kputp (ptr->next);
    if (ptr->available) {
      kputs (" Free\n");
    }
    else {
      kputs (" Used\n");
    }

    dump_heap_int (ptr->next);
  }
}

void
dump_heap ()
{
  kassert (heap_mode == HEAP);

  kputs ("first = "); kputp (heap_first); kputs (" last = "); kputp (heap_last); kputs ("\n");
  kputs ("Node       Magic      Size       Prev       Next\n");
  dump_heap_int (heap_first);
}

static void
split (header_t* ptr,
       unsigned int size)
{
  kassert (ptr != 0);
  kassert (ptr->available);
  kassert (ptr->size > sizeof (header_t) + size);

  /* Split the block. */
  header_t* n = (header_t*)((char*)ptr + sizeof (header_t) + size);
  n->available = 1;
  n->magic = HEAP_MAGIC;
  n->size = ptr->size - size - sizeof (header_t);
  n->prev = ptr;
  n->next = ptr->next;
  
  ptr->size = size;
  ptr->next = n;
  
  if (n->next != 0) {
    n->next->prev = n;
  }
  
  if (ptr == heap_last) {
    heap_last = n;
  }
}

static void
expand_heap (unsigned int size)
{
  while (heap_last->size < size) {
    /* Map it. */
    insert_mapping (kernel_page_directory, heap_limit, make_page_table_entry (frame_manager_allocate (), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), make_page_directory_entry (0, PAGE_SIZE_4K, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT));
    /* Update sizes and limits. */
    heap_limit += PAGE_SIZE;
    heap_last->size += PAGE_SIZE;

    /* Replace the spare page table if necessary. */
    if (spare_page_table == 0) {
      spare_page_table = allocate_page_table ();
    }
  }
}

static header_t*
find_header (header_t* start,
	     unsigned int size)
{
  for (; start != 0 && !(start->available && start->size >= size); start = start->next) ;;
  return start;
}

void*
kmalloc (unsigned int size)
{
  kassert (heap_mode == HEAP);

  header_t* ptr = find_header (heap_first, size);
  while (ptr == 0) {
    expand_heap (size);
    ptr = find_header (heap_last, size);
  }

  /* Found something we can use. */
  if ((ptr->size - size) > sizeof (header_t)) {
    split (ptr, size);
  }
  /* Mark as used and return. */
  ptr->available = 0;
  return (ptr + 1);
}

static header_t*
find_header_pa (header_t* start,
		unsigned int size,
		unsigned int* base,
		unsigned int* limit)
{
  while (start != 0) {
    if (start->available && start->size >= size) {
      *base = PAGE_ALIGN ((unsigned int)start + sizeof (header_t) + PAGE_SIZE - 1);
      *limit = (unsigned int)start + sizeof (header_t) + start->size;
      if ((*limit - *base) >= size) {
	break;
      }
    }
    start = start->next;
  }
  return start;
}

void*
kmalloc_pa (unsigned int size)
{
  kassert (heap_mode == HEAP);

  unsigned int base;
  unsigned int limit;

  header_t* ptr = find_header_pa (heap_first, size, &base, &limit);
  while (ptr == 0) {
    expand_heap (size);
    ptr = find_header_pa (heap_last, size, &base, &limit);
  }

  if (((unsigned int)ptr + sizeof (header_t)) != base) {
    if ((base - sizeof (header_t)) > ((unsigned int)ptr + sizeof (header_t))) {
      /* Split. */
      split (ptr, ptr->size - sizeof (header_t) - (limit - base));
      /* Change ptr to aligned block. */
      ptr = ptr->next;
    }
    else {
      /* Slide the header to align it with base. */
      unsigned int amount = base - ((unsigned int)ptr + sizeof (header_t));
      header_t* p = ptr->prev;
      header_t* n = ptr->next;
      unsigned int s = ptr->size;
      header_t* old = ptr;
      
      ptr = (header_t*) ((unsigned int)ptr + amount);
      ptr->available = 1;
      ptr->magic = HEAP_MAGIC;
      ptr->size = s - amount;
      ptr->prev = p;
      ptr->next = n;
      
      if (ptr->prev != 0) {
	ptr->prev->next = ptr;
	ptr->prev->size += amount;
      }
      
      if (ptr->next != 0) {
	ptr->next->prev = ptr;
      }
      
      if (heap_first == old) {
	heap_first = ptr;
      }
      
      if (heap_last == old) {
	heap_last = ptr;
      }
    }
  }
  
  if ((ptr->size - size) > sizeof (header_t)) {
    split (ptr, size);
  }
  /* Mark as used and return. */
  ptr->available = 0;
  return (ptr + 1);
}


void
kfree (void* p)
{
  header_t* ptr = p;
  --ptr;
  kassert ((unsigned int)ptr >= heap_base);
  kassert ((unsigned int)ptr < heap_limit);
  kassert (ptr->magic == HEAP_MAGIC);
  kassert (ptr->available == 0);

  ptr->available = 1;

  /* Merge with next. */
  if (ptr->next != 0 && ptr->next->available) {
    ptr->size += sizeof (header_t) + ptr->next->size;
    if (ptr->next == heap_last) {
      heap_last = ptr;
    }
    ptr->next = ptr->next->next;
    ptr->next->prev = ptr;
  }

  /* Merge with prev. */
  if (ptr->prev != 0 && ptr->prev->available) {
    ptr->prev->size += sizeof (header_t) + ptr->size;
    ptr->prev->next = ptr->next;
    ptr->next->prev = ptr->prev;
    if (ptr == heap_last) {
      heap_last = ptr->prev;
    }
  }
}

/* TODO:  Reclaim identity mapped memory. */
/* TODO:  Reclaim the stack. */
