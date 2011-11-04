/*
  File
  ----
  malloc.c
  
  Description
  -----------
  Memory allocator.

  Authors:
  Justin R. Wilson
*/

#include "malloc.h"
/* This will need to be removed later. */
#include "kassert.h"
#include "syscall.h"

#define MAGIC 0x7ACEDEAD

typedef struct header header_t;
struct header {
  unsigned int available : 1;
  unsigned int magic : 31;
  unsigned int size; /* Does not include header. */
  header_t* prev;
  header_t* next;
};

static unsigned int page_size = 0;
static header_t* first_header = 0;
static header_t* last_header = 0;

void*
malloc (unsigned int size)
{
  if (page_size == 0) {
    /* Initialize. */
    page_size = sys_get_page_size ();
    first_header = sys_allocate (page_size);
    last_header = first_header;
    first_header->available = 1;
    first_header->magic = MAGIC;
    first_header->size = page_size - sizeof (header_t);
    first_header->prev = 0;
    first_header->next = 0;
  }

  kassert (0);
}

void
free (void* ptr)
{
  kassert (0);
}




/* #ifndef __mm_h__ */
/* #define __mm_h__ */

/* /\* */
/*   File */
/*   ---- */
/*   memory.h */
  
/*   Description */
/*   ----------- */
/*   Declarations for the memory manager. */

/*   Authors: */
/*   http://wiki.osdev.org/Higher_Half_With_GDT */
/*   Justin R. Wilson */
/* *\/ */

/* #include "multiboot.h" */

/* #define USER_STACK_LIMIT 0x0 */

/* typedef struct page_directory page_directory_t; */

/* void */
/* initialize_memory_manager (const multiboot_info_t* mbd); */

/* void */
/* dump_heap (void); */

/* void* */
/* kmalloc (unsigned int size); */

/* /\* Page aligned. *\/ */
/* void* */
/* kmalloc_pa (unsigned int size); */

/* void */
/* kfree (void*); */

/* #endif /\* __mm_h__ *\/ */


/* #define USER_STACK_INITIAL_SIZE PAGE_SIZE */


/* static page_table_t* spare_page_table = 0; */

/* static unsigned int heap_base; */
/* static unsigned int heap_limit; */
/* static unsigned int user_stack_base; */

/* static header_t* heap_first; */
/* static header_t* heap_last; */

/* static page_table_t* */
/* allocate_page_table () */
/* { */
/*   page_table_t* ptr = kmalloc_pa (sizeof (page_table_t)); */
/*   initialize_page_table (ptr); */
/*   return ptr; */
/* } */

/* static unsigned int */
/* logical_to_physical (page_directory_t* ptr, */
/* 		     unsigned int logical_addr) */
/* { */
/*   kassert (ptr != 0); */

/*   /\* page_table_t* pt = ptr->entries_logical[PAGE_DIRECTORY_ENTRY (logical_addr)]; *\/ */

/*   /\* if (pt != 0) { *\/ */
/*   /\*   return FRAME_TO_ADDRESS (pt->entry[PAGE_TABLE_ENTRY (logical_addr)].frame) | PAGE_ADDRESS_OFFSET (logical_addr); *\/ */
/*   /\* } *\/ */
/*   /\* else { *\/ */
/*   /\*   return 0; *\/ */
/*   /\* } *\/ */
/* } */

/* static page_directory_t* */
/* allocate_page_directory () */
/* { */
/*   page_directory_t* ptr = kmalloc_pa (sizeof (page_directory_t)); */
/*   /\* Copy from the kernel. *\/ */
/*   unsigned int k; */
/*   for (k = 0; k < PAGE_ENTRY_COUNT; ++k) { */
/*     ptr->entry[k] = kernel_page_directory.entry[k]; */
/*   } */

/*   return ptr; */
/* } */

/* static void */
/* erase_mapping (page_directory_t* ptr, */
/* 	       unsigned int logical_addr) */
/* { */
/*   kassert (0); */
/* } */

/* void */
/* initialize_memory_manager (const multiboot_info_t* mbd) */
/* { */
/*   /\* Clear the page directory and page table. *\/ */
/*   initialize_page_table (&low_page_table); */
/*   initialize_page_directory (&kernel_page_directory, (unsigned int)&kernel_page_directory - KERNEL_VIRTUAL_BASE); */

/*   /\* Insert the page table. *\/ */
/*   unsigned int low_page_table_paddr = (unsigned int)&low_page_table - KERNEL_VIRTUAL_BASE; */
/*   insert_page_table (&kernel_page_directory, 0, make_page_directory_entry (ADDRESS_TO_FRAME (low_page_table_paddr), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), &low_page_table); */
/*   insert_page_table (&kernel_page_directory, KERNEL_VIRTUAL_BASE, make_page_directory_entry (ADDRESS_TO_FRAME (low_page_table_paddr), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), &low_page_table); */

/*   /\* Identity map the first 1M. *\/ */
/*   unsigned int physical_addr; */
/*   for (physical_addr = 0; physical_addr < 0x100000; physical_addr += PAGE_SIZE) { */
/*     insert_mapping (&kernel_page_directory, physical_addr, make_page_table_entry (ADDRESS_TO_FRAME (physical_addr), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), make_page_directory_entry (0, 0, 0, 0, 0, 0, 0)); */
/*   } */

/*   /\* Identity map the kernel. *\/ */
/*   kassert (IS_PAGE_ALIGNED ((unsigned int)&kernel_begin)); */
/*   kassert (((unsigned int)&kernel_end - KERNEL_VIRTUAL_BASE) <= IDENTITY_MAP_LIMIT); */
/*   for (physical_addr = (unsigned int)&kernel_begin - KERNEL_VIRTUAL_BASE; physical_addr < (unsigned int)&kernel_end - KERNEL_VIRTUAL_BASE; physical_addr += PAGE_SIZE) { */
/*     insert_mapping (&kernel_page_directory, physical_addr + KERNEL_VIRTUAL_BASE, make_page_table_entry (ADDRESS_TO_FRAME (physical_addr), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), make_page_directory_entry (0, 0, 0, 0, 0, 0, 0)); */
/*   } */

/*   switch_to_page_directory (&kernel_page_directory); */

/*   unsigned int identity_begin = physical_addr; */
/*   unsigned int identity_end = identity_begin; */

/*   /\* Extend memory to cover the GRUB data structures. *\/ */
/*   extend_identity_map ((unsigned int)mbd + sizeof (multiboot_info_t) - 1, &identity_end); */

/*   if (mbd->flags & MULTIBOOT_INFO_MEMORY) { */
/*     kputs ("mem_lower: "); kputuix (mbd->mem_lower); kputs ("\n"); */
/*     kputs ("mem_upper: "); kputuix (mbd->mem_upper); kputs ("\n"); */
/*   } */

/*   if (mbd->flags & MULTIBOOT_INFO_BOOTDEV) { */
/*     kputs ("boot_device: "); kputuix (mbd->boot_device); kputs ("\n"); */
/*   } */

/*   if (mbd->flags & MULTIBOOT_INFO_CMDLINE) { */
/*     char* cmdline = (char*)mbd->cmdline; */
/*     extend_identity_map ((unsigned int)cmdline, &identity_end); */
/*     for (; *cmdline != 0; ++cmdline) { */
/*       extend_identity_map ((unsigned int)cmdline, &identity_end); */
/*     } */
/*     kputs ("cmdline: "); kputs ((char *)mbd->cmdline); kputs ("\n"); */
/*   } */

/*   /\* if (mbd->flags & MULTIBOOT_INFO_MODS) { *\/ */
/*   /\*   kputs ("MULTIBOOT_INFO_MODS\n"); *\/ */
/*   /\* } *\/ */
/*   /\* if (mbd->flags & MULTIBOOT_INFO_AOUT_SYMS) { *\/ */
/*   /\*   kputs ("MULTIBOOT_INFO_INFO_AOUT_SYMS\n"); *\/ */
/*   /\* } *\/ */
/*   /\* if (mbd->flags & MULTIBOOT_INFO_ELF_SHDR) { *\/ */
/*   /\*   kputs ("MULTIBOOT_INFO_ELF_SHDR\n"); *\/ */
/*   /\* } *\/ */

/*   if (mbd->flags & MULTIBOOT_INFO_MEM_MAP) { */
/*     extend_identity_map (mbd->mmap_addr + mbd->mmap_length - 1, &identity_end); */
/*   } */

/*   /\* if (mbd->flags & MULTIBOOT_INFO_DRIVE_INFO) { *\/ */
/*   /\*   kputs ("MULTIBOOT_INFO_DRIVE_INFO\n"); *\/ */
/*   /\* } *\/ */
/*   /\* if (mbd->flags & MULTIBOOT_INFO_CONFIG_TABLE) { *\/ */
/*   /\*   kputs ("MULTIBOOT_INFO_CONFIG_TABLE\n"); *\/ */
/*   /\* } *\/ */

/*   if (mbd->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) { */
/*     char* name = (char*)mbd->boot_loader_name; */
/*     extend_identity_map ((unsigned int)name, &identity_end); */
/*     for (; *name != 0; ++name) { */
/*       extend_identity_map ((unsigned int)name, &identity_end); */
/*     } */
/*     kputs ("boot_loader_name: "); kputs ((char *)mbd->boot_loader_name); kputs ("\n"); */
/*   } */

/*   /\* if (mbd->flags & MULTIBOOT_INFO_APM_TABLE) { *\/ */
/*   /\*   kputs ("MULTIBOOT_INFO_APM_TABLE\n"); *\/ */
/*   /\* } *\/ */
/*   /\* if (mbd->flags & MULTIBOOT_INFO_VIDEO_INFO) { *\/ */
/*   /\*   kputs ("MULTIBOOT_INFO_VIDEO_INFO\n"); *\/ */
/*   /\* } *\/ */

/*   unsigned int placement_begin = identity_end + KERNEL_VIRTUAL_BASE; */
/*   unsigned int placement_end = placement_begin; */

/*   /\* Parse the multiboot data structure. *\/ */
/*   multiboot_memory_map_t* ptr = (multiboot_memory_map_t*)mbd->mmap_addr; */
/*   multiboot_memory_map_t* limit = (multiboot_memory_map_t*)(mbd->mmap_addr + mbd->mmap_length); */
/*   while (ptr < limit) { */
/*     unsigned int first = ptr->addr; */
/*     unsigned int last = ptr->addr + ptr->len - 1; */
/*     kputuix (first); kputs ("-"); kputuix (last); */
/*     switch (ptr->type) { */
/*     case MULTIBOOT_MEMORY_AVAILABLE: */
/*       kputs (" AVAILABLE\n"); */
/*       /\* Don't mess with memory below a certain limit. *\/ */
/*       if (last > MEMORY_LOWER_LIMIT) { */
/* 	if (first < MEMORY_LOWER_LIMIT) { */
/* 	  first = MEMORY_LOWER_LIMIT; */
/* 	} */
/* 	/\* Split memory that straddles the DMA limit. *\/ */
/* 	if (first < MEMORY_DMA_LIMIT && last >= MEMORY_DMA_LIMIT) { */
/* 	  allocate_dma_zone (first, MEMORY_DMA_LIMIT - 1); */
/* 	  allocate_normal_zone (MEMORY_DMA_LIMIT, last, &placement_end); */
/* 	} */
/* 	else if (last < MEMORY_DMA_LIMIT) { */
/* 	  allocate_dma_zone (first, last); */
/* 	} */
/* 	else { */
/* 	  allocate_normal_zone (first, last, &placement_end); */
/* 	} */
/*       } */
/*       break; */
/*     case MULTIBOOT_MEMORY_RESERVED: */
/*       kputs (" RESERVED\n"); */
/*       break; */
/*     default: */
/*       kputs (" UNKNOWN\n"); */
/*       break; */
/*     } */
/*     /\* Move to the next descriptor. *\/ */
/*     ptr = (multiboot_memory_map_t*) (((char*)&(ptr->addr)) + ptr->size); */
/*   } */

/*   /\* We are done with the identity mapped region. *\/ */
/*   for (; identity_begin < identity_end; identity_begin += PAGE_SIZE) { */
/*     erase_mapping (&kernel_page_directory, identity_begin); */
/*   } */



/*   /\* /\\* All of the frames (physical pages) up to identity_end are in use. *\\/ *\/ */
/*   /\* unsigned int frame; *\/ */
/*   /\* /\\* for (frame = 0; frame < ADDRESS_TO_FRAME (identity_end); ++frame) { *\\/ *\/ */
/*   /\* /\\*   frame_manager_set (frame); *\\/ *\/ */
/*   /\* /\\* } *\\/ *\/ */

/*   kputs ("placement_begin "); kputuix (placement_begin); kputs ("\n"); */
/*   kputs ("placement_end "); kputuix (placement_end); kputs ("\n"); */
/* } */













/* void */
/* initialize_heap (const multiboot_info_t* mbd) */
/* { */
/*   /\* heap_base = identity_end + KERNEL_VIRTUAL_BASE; *\/ */
/*   /\* heap_limit = heap_base; *\/ */

/*   /\* Grow the heap to its initial size. *\/ */
/*   for (; (heap_limit - heap_base) < HEAP_INITIAL_SIZE; heap_limit += PAGE_SIZE) { */
/*     //insert_mapping (&kernel_page_directory, heap_limit, make_page_table_entry (frame_manager_allocate (), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), make_page_directory_entry (0, PAGE_SIZE_4K, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT)); */
/*   } */

/*   /\* Imprint the heap. *\/ */
/*   heap_first = (header_t*)heap_base; */
/*   heap_last = heap_first; */

/*   heap_first->available = 1; */
/*   heap_first->magic = HEAP_MAGIC; */
/*   heap_first->size = heap_limit - heap_base - sizeof (header_t); */
/*   heap_first->prev = 0; */
/*   heap_first->next = 0; */

/*   /\* Allocate a spare page table so we can always exand the kernel's address space. *\/ */
/*   spare_page_table = allocate_page_table (); */

/*   /\* Initialize the stack. *\/ */
/*   user_stack_base = USER_STACK_LIMIT - USER_STACK_INITIAL_SIZE; */

/*   unsigned int stack_addr; */
/*   /\* Use != due to wrap around. *\/ */
/*   for (stack_addr = user_stack_base; stack_addr != USER_STACK_LIMIT; stack_addr += PAGE_SIZE) { */
/*     //insert_mapping (&kernel_page_directory, stack_addr, make_page_table_entry (frame_manager_allocate (), NOT_GLOBAL, CACHED, WRITE_BACK, USER, WRITABLE, PRESENT), make_page_directory_entry (0, PAGE_SIZE_4K, CACHED, WRITE_BACK, USER, WRITABLE, PRESENT)); */
/*   } */
  
/*   /\* kputs ("Total frames: "); kputuix (total_frames); kputs ("\n"); *\/ */
/*   /\* kputs ("Used frames: "); kputuix (used_frames); kputs ("\n"); *\/ */

/*   /\* kputs ("identity_begin: "); kputuix (identity_begin); kputs ("\n"); *\/ */
/*   /\* kputs ("identity_end: "); kputuix (identity_end); kputs ("\n"); *\/ */
/*   /\* kputs ("placement_base: "); kputuix (placement_base); kputs ("\n"); *\/ */
/*   /\* kputs ("placement_limit: "); kputuix (placement_limit); kputs ("\n"); *\/ */
/*   /\* kputs ("heap_base: "); kputuix (heap_base); kputs ("\n"); *\/ */
/*   /\* kputs ("heap_limit: "); kputuix (heap_limit); kputs ("\n"); *\/ */
/*   /\* kputs ("user_stack_base: "); kputuix (user_stack_base); kputs ("\n"); *\/ */
/*   /\* kputs ("user_stack_limit: "); kputuix (user_stack_limit); kputs ("\n"); *\/ */
/* } */

/* static void */
/* dump_heap_int (header_t* ptr) */
/* { */
/*   if (ptr != 0) { */
/*     kputp (ptr); kputs (" "); kputuix (ptr->magic); kputs (" "); kputuix (ptr->size); kputs (" "); kputp (ptr->prev); kputs (" "); kputp (ptr->next); */
/*     if (ptr->available) { */
/*       kputs (" Free\n"); */
/*     } */
/*     else { */
/*       kputs (" Used\n"); */
/*     } */

/*     dump_heap_int (ptr->next); */
/*   } */
/* } */

/* void */
/* dump_heap () */
/* { */
/*   kputs ("first = "); kputp (heap_first); kputs (" last = "); kputp (heap_last); kputs ("\n"); */
/*   kputs ("Node       Magic      Size       Prev       Next\n"); */
/*   dump_heap_int (heap_first); */
/* } */

/* static void */
/* split (header_t* ptr, */
/*        unsigned int size) */
/* { */
/*   kassert (ptr != 0); */
/*   kassert (ptr->available); */
/*   kassert (ptr->size > sizeof (header_t) + size); */

/*   /\* Split the block. *\/ */
/*   header_t* n = (header_t*)((char*)ptr + sizeof (header_t) + size); */
/*   n->available = 1; */
/*   n->magic = HEAP_MAGIC; */
/*   n->size = ptr->size - size - sizeof (header_t); */
/*   n->prev = ptr; */
/*   n->next = ptr->next; */
  
/*   ptr->size = size; */
/*   ptr->next = n; */
  
/*   if (n->next != 0) { */
/*     n->next->prev = n; */
/*   } */
  
/*   if (ptr == heap_last) { */
/*     heap_last = n; */
/*   } */
/* } */

/* static void */
/* expand_heap (unsigned int size) */
/* { */
/*   while (heap_last->size < size) { */
/*     /\* Map it. *\/ */
/*     //insert_mapping (&kernel_page_directory, heap_limit, make_page_table_entry (frame_manager_allocate (), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), make_page_directory_entry (0, PAGE_SIZE_4K, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT)); */
/*     /\* Update sizes and limits. *\/ */
/*     heap_limit += PAGE_SIZE; */
/*     heap_last->size += PAGE_SIZE; */

/*     /\* Replace the spare page table if necessary. *\/ */
/*     if (spare_page_table == 0) { */
/*       spare_page_table = allocate_page_table (); */
/*     } */
/*   } */
/* } */

/* static header_t* */
/* find_header (header_t* start, */
/* 	     unsigned int size) */
/* { */
/*   for (; start != 0 && !(start->available && start->size >= size); start = start->next) ;; */
/*   return start; */
/* } */

/* void* */
/* kmalloc (unsigned int size) */
/* { */
/*   header_t* ptr = find_header (heap_first, size); */
/*   while (ptr == 0) { */
/*     expand_heap (size); */
/*     ptr = find_header (heap_last, size); */
/*   } */

/*   /\* Found something we can use. *\/ */
/*   if ((ptr->size - size) > sizeof (header_t)) { */
/*     split (ptr, size); */
/*   } */
/*   /\* Mark as used and return. *\/ */
/*   ptr->available = 0; */
/*   return (ptr + 1); */
/* } */

/* static header_t* */
/* find_header_pa (header_t* start, */
/* 		unsigned int size, */
/* 		unsigned int* base, */
/* 		unsigned int* limit) */
/* { */
/*   while (start != 0) { */
/*     if (start->available && start->size >= size) { */
/*       *base = PAGE_ALIGN ((unsigned int)start + sizeof (header_t) + PAGE_SIZE - 1); */
/*       *limit = (unsigned int)start + sizeof (header_t) + start->size; */
/*       if ((*limit - *base) >= size) { */
/* 	break; */
/*       } */
/*     } */
/*     start = start->next; */
/*   } */
/*   return start; */
/* } */

/* void* */
/* kmalloc_pa (unsigned int size) */
/* { */
/*   unsigned int base; */
/*   unsigned int limit; */

/*   header_t* ptr = find_header_pa (heap_first, size, &base, &limit); */
/*   while (ptr == 0) { */
/*     expand_heap (size); */
/*     ptr = find_header_pa (heap_last, size, &base, &limit); */
/*   } */

/*   if (((unsigned int)ptr + sizeof (header_t)) != base) { */
/*     if ((base - sizeof (header_t)) > ((unsigned int)ptr + sizeof (header_t))) { */
/*       /\* Split. *\/ */
/*       split (ptr, ptr->size - sizeof (header_t) - (limit - base)); */
/*       /\* Change ptr to aligned block. *\/ */
/*       ptr = ptr->next; */
/*     } */
/*     else { */
/*       /\* Slide the header to align it with base. *\/ */
/*       unsigned int amount = base - ((unsigned int)ptr + sizeof (header_t)); */
/*       header_t* p = ptr->prev; */
/*       header_t* n = ptr->next; */
/*       unsigned int s = ptr->size; */
/*       header_t* old = ptr; */
      
/*       ptr = (header_t*) ((unsigned int)ptr + amount); */
/*       ptr->available = 1; */
/*       ptr->magic = HEAP_MAGIC; */
/*       ptr->size = s - amount; */
/*       ptr->prev = p; */
/*       ptr->next = n; */
      
/*       if (ptr->prev != 0) { */
/* 	ptr->prev->next = ptr; */
/* 	ptr->prev->size += amount; */
/*       } */
      
/*       if (ptr->next != 0) { */
/* 	ptr->next->prev = ptr; */
/*       } */
      
/*       if (heap_first == old) { */
/* 	heap_first = ptr; */
/*       } */
      
/*       if (heap_last == old) { */
/* 	heap_last = ptr; */
/*       } */
/*     } */
/*   } */
  
/*   if ((ptr->size - size) > sizeof (header_t)) { */
/*     split (ptr, size); */
/*   } */
/*   /\* Mark as used and return. *\/ */
/*   ptr->available = 0; */
/*   return (ptr + 1); */
/* } */


/* void */
/* kfree (void* p) */
/* { */
/*   header_t* ptr = p; */
/*   --ptr; */
/*   kassert ((unsigned int)ptr >= heap_base); */
/*   kassert ((unsigned int)ptr < heap_limit); */
/*   kassert (ptr->magic == HEAP_MAGIC); */
/*   kassert (ptr->available == 0); */

/*   ptr->available = 1; */

/*   /\* Merge with next. *\/ */
/*   if (ptr->next != 0 && ptr->next->available) { */
/*     ptr->size += sizeof (header_t) + ptr->next->size; */
/*     if (ptr->next == heap_last) { */
/*       heap_last = ptr; */
/*     } */
/*     ptr->next = ptr->next->next; */
/*     ptr->next->prev = ptr; */
/*   } */

/*   /\* Merge with prev. *\/ */
/*   if (ptr->prev != 0 && ptr->prev->available) { */
/*     ptr->prev->size += sizeof (header_t) + ptr->size; */
/*     ptr->prev->next = ptr->next; */
/*     ptr->next->prev = ptr->prev; */
/*     if (ptr == heap_last) { */
/*       heap_last = ptr->prev; */
/*     } */
/*   } */
/* } */

/* /\* TODO:  Reclaim identity mapped memory. *\/ */
/* /\* TODO:  Reclaim the stack. *\/ */
