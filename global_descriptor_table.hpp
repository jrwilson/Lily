#ifndef __global_descriptor_table_hpp__
#define __global_descriptor_table_hpp__

/*
  File
  ----
  gdt.h
  
  Description
  -----------
  Global Descriptor Table (GDT) object.

  Authors:
  Justin R. Wilson
*/

#include "descriptor.hpp"
#include "types.hpp"

/* Should be consistent with segments.s. */
static const unsigned char DESCRIPTOR_COUNT = 5;
static const unsigned char NULL_SELECTOR = 0x00;
static const unsigned char KERNEL_CODE_SELECTOR = 0x08;
static const unsigned char KERNEL_DATA_SELECTOR = 0x10;
static const unsigned char USER_CODE_SELECTOR = 0x18;
static const unsigned char USER_DATA_SELECTOR = 0x20;

struct gdt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

extern "C" void
gdt_flush (gdt_ptr*);

class global_descriptor_table {
private:
  gdt_ptr gp_;
  descriptor gdt_entry_[DESCRIPTOR_COUNT];

public:
  global_descriptor_table ()
  {
    gp_.limit = (sizeof (descriptor) * DESCRIPTOR_COUNT) - 1;
    gp_.base = reinterpret_cast<uint32_t> (gdt_entry_);
    
    gdt_entry_[NULL_SELECTOR / sizeof (descriptor)] = make_null_descriptor ();
    gdt_entry_[KERNEL_CODE_SELECTOR / sizeof (descriptor)].code_segment = make_code_segment_descriptor (0, 0xFFFFFFFF, READABLE, NOT_CONFORMING, RING0, PRESENT, WIDTH_32, PAGE_GRANULARITY); 
    gdt_entry_[KERNEL_DATA_SELECTOR / sizeof (descriptor)].data_segment = make_data_segment_descriptor (0, 0xFFFFFFFF, WRITABLE, EXPAND_UP, RING0, PRESENT, WIDTH_32, PAGE_GRANULARITY);
    gdt_entry_[USER_CODE_SELECTOR / sizeof (descriptor)].code_segment = make_code_segment_descriptor (0, 0xFFFFFFFF, READABLE, NOT_CONFORMING, RING3, PRESENT, WIDTH_32, PAGE_GRANULARITY);
    gdt_entry_[USER_DATA_SELECTOR / sizeof (descriptor)].data_segment = make_data_segment_descriptor (0, 0xFFFFFFFF, WRITABLE, EXPAND_UP, RING3, PRESENT, WIDTH_32, PAGE_GRANULARITY);
    
    gdt_flush (&gp_);
  }
};

#endif /* __global_descriptor_table_hpp__ */
