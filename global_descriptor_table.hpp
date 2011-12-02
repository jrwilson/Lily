#ifndef __global_descriptor_table_hpp__
#define __global_descriptor_table_hpp__

/*
  File
  ----
  global_descriptor_table.hpp
  
  Description
  -----------
  Global Descriptor Table (GDT) object.

  Authors:
  Justin R. Wilson
*/

#include "descriptor.hpp"
#include "types.hpp"

/* Should be consistent with segments.s. */
const unsigned char DESCRIPTOR_COUNT = 5;
const unsigned char NULL_SELECTOR = 0x00;
const unsigned char KERNEL_CODE_SELECTOR = 0x08;
const unsigned char KERNEL_DATA_SELECTOR = 0x10;
const unsigned char USER_CODE_SELECTOR = 0x18;
const unsigned char USER_DATA_SELECTOR = 0x20;

struct gdt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

class global_descriptor_table {
private:
  gdt_ptr gp_;
  descriptor gdt_entry_[DESCRIPTOR_COUNT];

public:
  global_descriptor_table ();
};

#endif /* __global_descriptor_table_hpp__ */
