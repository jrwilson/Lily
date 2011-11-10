/*
  File
  ----
  gdt.h
  
  Description
  -----------
  Functions for the Global Descriptor Table (GDT).

  Authors:
  Justin R. Wilson
*/

#include "gdt.hpp"
#include "descriptor.hpp"

struct gdt_ptr
{
  unsigned short limit;
  unsigned int base;
} __attribute__((packed));
typedef struct gdt_ptr gdt_ptr_t;

static descriptor_t gdt_entry[DESCRIPTOR_COUNT];
static gdt_ptr_t gp;

extern "C" void
gdt_flush (gdt_ptr_t*);

void
gdt_initialize (void)
{
  gp.limit = (sizeof (descriptor_t) * DESCRIPTOR_COUNT) - 1;
  gp.base = (unsigned int)gdt_entry;

  gdt_entry[0] = make_null_descriptor ();
  gdt_entry[KERNEL_CODE_SELECTOR / sizeof (descriptor_t)].code_segment = make_code_segment_descriptor (0, 0xFFFFFFFF, READABLE, NOT_CONFORMING, RING0, PRESENT, WIDTH_32, PAGE_GRANULARITY); 
  gdt_entry[KERNEL_DATA_SELECTOR / sizeof (descriptor_t)].data_segment = make_data_segment_descriptor (0, 0xFFFFFFFF, WRITABLE, EXPAND_UP, RING0, PRESENT, WIDTH_32, PAGE_GRANULARITY);
  gdt_entry[USER_CODE_SELECTOR / sizeof (descriptor_t)].code_segment = make_code_segment_descriptor (0, 0xFFFFFFFF, READABLE, NOT_CONFORMING, RING3, PRESENT, WIDTH_32, PAGE_GRANULARITY);
  gdt_entry[USER_DATA_SELECTOR / sizeof (descriptor_t)].data_segment = make_data_segment_descriptor (0, 0xFFFFFFFF, WRITABLE, EXPAND_UP, RING3, PRESENT, WIDTH_32, PAGE_GRANULARITY);

  gdt_flush (&gp);
}
