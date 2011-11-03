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
#include "kput.h"

/* Can identity map up to 4 megabytes. */
#define IDENTITY_MAP_LIMIT 0x400000

/* Don't mess with memory below 1M. */
#define MEMORY_LOWER_LIMIT 0x100000

/* Memory below this limit will be reserved for DMA. */
#define MEMORY_DMA_LIMIT 0x1000000

typedef enum {
  NORMAL_ZONE,
  DMA_ZONE,
} zone_type_t;

typedef enum {
  STACK_ALLOCATOR
} allocator_type_t;

typedef struct zone_item zone_item_t;
struct zone_item {
  zone_type_t zone_type;
  allocator_type_t allocator_type;
  zone_item_t* next;
};

#define STACK_ALLOCATOR_EOL 0x7FFFFFFF

typedef struct stack_allocator {
  /* From zone item. */
  zone_type_t zone_type;
  allocator_type_t allocator_type;
  zone_item_t* next;
  /* */
  unsigned int frame_begin;
  unsigned int frame_end;
  int free_head;
  /*
    Entry stores the next entry on the free list or the reference count.
    Next has the range [0, size - 1].  0x7FFFFFFF marks the end of the free list.
    Reference count is the additive inverse of the reference count.
  */
  int entry[];
} stack_allocator_t;

/* Marks the end of the kernel upon loading. */
extern int kernel_end;

static unsigned int identity_end;
static unsigned int placement_end;

/* Linked list of zone allocators. */
static zone_item_t* zone_head = 0;

static void
extend_identity_map (unsigned int addr)
{
  kassert (addr <= IDENTITY_MAP_LIMIT);
  kassert (identity_end <= IDENTITY_MAP_LIMIT);
  identity_end = ((identity_end) > addr) ? (identity_end) : addr;
}

static void*
placement_alloc (unsigned int size)
{
  char* retval = (char*)placement_end;
  placement_end += size;
  kassert (placement_end <= KERNEL_VIRTUAL_BASE + IDENTITY_MAP_LIMIT);
  return retval;
}

static zone_item_t*
allocate_stack_allocator (unsigned int first,
			  unsigned int last)
{
  unsigned int size = ADDRESS_TO_FRAME (last + 1 - first);
  stack_allocator_t* ptr = placement_alloc (sizeof (stack_allocator_t) + size * sizeof (int));
  ptr->allocator_type = STACK_ALLOCATOR;
  ptr->next = 0;
  ptr->frame_begin = ADDRESS_TO_FRAME (first);
  ptr->frame_end = ptr->frame_begin + size;
  ptr->free_head = 0;
  int k;
  for (k = 0; k < (int)size; ++k) {
    ptr->entry[k] = k + 1;
  }
  ptr->entry[size - 1] = STACK_ALLOCATOR_EOL;

  return (zone_item_t*)ptr;
}

static int
stack_allocator_mark_as_used (zone_item_t* p,
			      unsigned int frame)
{
  kassert (p != 0);
  kassert (p->allocator_type == STACK_ALLOCATOR);
  stack_allocator_t* ptr = (stack_allocator_t*)p;

  if (frame >= ptr->frame_begin && frame < ptr->frame_end) {
    int frame_idx = frame - ptr->frame_begin;
    /* Should be free. */
    kassert (ptr->entry[frame_idx] >= 0 && ptr->entry[frame_idx] != STACK_ALLOCATOR_EOL);
    /* Find the one that points to it. */
    int idx;
    for (idx = ptr->free_head; idx != STACK_ALLOCATOR_EOL && ptr->entry[idx] != frame_idx; idx = ptr->entry[idx]) ;;
    kassert (idx != STACK_ALLOCATOR_EOL && ptr->entry[idx] == frame_idx);
    /* Update the pointers. */
    ptr->entry[idx] = ptr->entry[frame_idx];
    ptr->entry[frame_idx] = -1;

    return 0;
  }
  else {
    return -1;
  }
}

static unsigned int
stack_allocator_allocate (zone_item_t* p)
{
  kassert (p != 0);
  kassert (p->allocator_type == STACK_ALLOCATOR);
  stack_allocator_t* ptr = (stack_allocator_t*)p;

  if (ptr->free_head != STACK_ALLOCATOR_EOL) {
    unsigned int idx = ptr->free_head;
    ptr->free_head = ptr->entry[idx];
    ptr->entry[idx] = -1;
    return idx + ptr->frame_begin;
  }
  else {
    return 0;
  }
}

static void
allocate_dma_zone (unsigned int first,
		   unsigned int last)
{
  /* DMA zone need a contiguous allocator like the buddy algorithm.
     For now, just use a stack allocator.
     TODO:  Implement buddy allocator. */
  zone_item_t* ptr = allocate_stack_allocator (first, last);
  ptr->zone_type = DMA_ZONE;
  /* DMA zones are added to the back of the list. */
  zone_item_t** p = &zone_head;
  for (; *p != 0 && (*p)->zone_type != DMA_ZONE; p = &(*p)->next) ;;
  ptr->next = *p;
  *p = ptr;
}

static void
allocate_normal_zone (unsigned int first,
		      unsigned int last)
{
  /* Normal zones use the stack allocator. */
  zone_item_t* ptr = allocate_stack_allocator (first, last); 
  ptr->zone_type = NORMAL_ZONE;
  /* Normal zones are added to the front of the list. */
  ptr->next = zone_head;
  zone_head = ptr;
}

void
frame_manager_initialize (const multiboot_info_t* mbd)
{
  /* The loader has mapped physical memory from 0x00000000-0x00400000 has been mapped to the logical addresses 0x00000000-0x00400000 and 0xC0000000-0xC0400000. */

  /* One past the last byte in identity map. */
  identity_end = PAGE_ALIGN ((unsigned int)&kernel_end - KERNEL_VIRTUAL_BASE + PAGE_SIZE - 1);

  /* Extend memory to cover the GRUB data structures. */
  extend_identity_map ((unsigned int)mbd + sizeof (multiboot_info_t));

  if (mbd->flags & MULTIBOOT_INFO_MEMORY) {
    kputs ("mem_lower: "); kputuix (mbd->mem_lower); kputs ("\n");
    kputs ("mem_upper: "); kputuix (mbd->mem_upper); kputs ("\n");
  }

  if (mbd->flags & MULTIBOOT_INFO_BOOTDEV) {
    kputs ("boot_device: "); kputuix (mbd->boot_device); kputs ("\n");
  }

  if (mbd->flags & MULTIBOOT_INFO_CMDLINE) {
    char* cmdline = (char*)mbd->cmdline;
    extend_identity_map ((unsigned int)cmdline + 1);
    for (; *cmdline != 0; ++cmdline) {
      extend_identity_map ((unsigned int)cmdline + 1);
    }
    kputs ("cmdline: "); kputs ((char *)mbd->cmdline); kputs ("\n");
  }

  /* if (mbd->flags & MULTIBOOT_INFO_MODS) { */
  /*   kputs ("MULTIBOOT_INFO_MODS\n"); */
  /* } */
  /* if (mbd->flags & MULTIBOOT_INFO_AOUT_SYMS) { */
  /*   kputs ("MULTIBOOT_INFO_INFO_AOUT_SYMS\n"); */
  /* } */
  /* if (mbd->flags & MULTIBOOT_INFO_ELF_SHDR) { */
  /*   kputs ("MULTIBOOT_INFO_ELF_SHDR\n"); */
  /* } */

  if (mbd->flags & MULTIBOOT_INFO_MEM_MAP) {
    extend_identity_map (mbd->mmap_addr + mbd->mmap_length);
  }

  /* if (mbd->flags & MULTIBOOT_INFO_DRIVE_INFO) { */
  /*   kputs ("MULTIBOOT_INFO_DRIVE_INFO\n"); */
  /* } */
  /* if (mbd->flags & MULTIBOOT_INFO_CONFIG_TABLE) { */
  /*   kputs ("MULTIBOOT_INFO_CONFIG_TABLE\n"); */
  /* } */

  if (mbd->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
    char* name = (char*)mbd->boot_loader_name;
    extend_identity_map ((unsigned int)name + 1);
    for (; *name != 0; ++name) {
      extend_identity_map ((unsigned int)name + 1);
    }
    kputs ("boot_loader_name: "); kputs ((char *)mbd->boot_loader_name); kputs ("\n");
  }

  /* if (mbd->flags & MULTIBOOT_INFO_APM_TABLE) { */
  /*   kputs ("MULTIBOOT_INFO_APM_TABLE\n"); */
  /* } */
  /* if (mbd->flags & MULTIBOOT_INFO_VIDEO_INFO) { */
  /*   kputs ("MULTIBOOT_INFO_VIDEO_INFO\n"); */
  /* } */

  /* Align the end of the identity map. */
  identity_end = PAGE_ALIGN (identity_end + PAGE_SIZE - 1);
  /* TODO:  Memory from [identity_begin, identity_end) can be reclaimed after the multiboot data structures are processed. */

  /* Marker for a placement allocator. */
  placement_end = identity_end + KERNEL_VIRTUAL_BASE;

  /* Parse the multiboot data structure. */
  multiboot_memory_map_t* ptr = (multiboot_memory_map_t*)mbd->mmap_addr;
  multiboot_memory_map_t* limit = (multiboot_memory_map_t*)(mbd->mmap_addr + mbd->mmap_length);
  while (ptr < limit) {
    unsigned int first = ptr->addr;
    unsigned int last = ptr->addr + ptr->len - 1;
    kputuix (first); kputs ("-"); kputuix (last);
    switch (ptr->type) {
    case MULTIBOOT_MEMORY_AVAILABLE:
      kputs (" AVAILABLE\n");
      /* Don't mess with memory below a certain limit. */
      if (last > MEMORY_LOWER_LIMIT) {
  	if (first < MEMORY_LOWER_LIMIT) {
  	  first = MEMORY_LOWER_LIMIT;
  	}
  	/* Split memory that straddles the DMA limit. */
  	if (first < MEMORY_DMA_LIMIT && last >= MEMORY_DMA_LIMIT) {
  	  allocate_dma_zone (first, MEMORY_DMA_LIMIT - 1);
  	  allocate_normal_zone (MEMORY_DMA_LIMIT, last);
  	}
  	else if (last < MEMORY_DMA_LIMIT) {
  	  allocate_dma_zone (first, last);
  	}
  	else {
  	  allocate_normal_zone (first, last);
  	}
      }
      break;
    case MULTIBOOT_MEMORY_RESERVED:
      kputs (" RESERVED\n");
      break;
    default:
      kputs (" UNKNOWN\n");
      break;
    }
    /* Move to the next descriptor. */
    ptr = (multiboot_memory_map_t*) (((char*)&(ptr->addr)) + ptr->size);
  }

  /* The logical address space can be expanded starting from placement_end. */
  placement_end = PAGE_ALIGN (placement_end + PAGE_SIZE - 1);
}

unsigned int
frame_manager_physical_begin (void)
{
  return identity_end;
}

unsigned int
frame_manager_physical_end (void)
{
  return placement_end - KERNEL_VIRTUAL_BASE;
}

void
frame_manager_mark_as_used (unsigned int frame)
{
  zone_item_t* ptr;
  for (ptr = zone_head; ptr != 0; ptr = ptr->next) {
    switch (ptr->allocator_type) {
    case STACK_ALLOCATOR:
      if (stack_allocator_mark_as_used (ptr, frame) == 0) {
	return;
      }
      break;
    }
  }
}

unsigned int
frame_manager_allocate ()
{
  unsigned int retval = 0;

  zone_item_t* ptr;
  for (ptr = zone_head; ptr != 0; ptr = ptr->next) {
    switch (ptr->allocator_type) {
    case STACK_ALLOCATOR:
      retval = stack_allocator_allocate (ptr);
      if (retval != 0) {
	return retval;
      }
      break;
    }
  }
  
  /* Out of physical memory. */
  kassert (0);
  return 0;
}
