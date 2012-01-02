#ifndef __gdt_hpp__
#define __gdt_hpp__

/*
  File
  ----
  gdt.hpp
  
  Description
  -----------
  Global Descriptor Table (GDT) object.

  Authors:
  Justin R. Wilson
*/

#include <stdint.h>
#include "descriptor.hpp"

class gdt {
public:
  struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
  } __attribute__((packed));

  static const unsigned char KERNEL_CODE_SELECTOR = 0x08;
  static const unsigned char KERNEL_DATA_SELECTOR = 0x10;
  static const unsigned char USER_CODE_SELECTOR = 0x18;
  static const unsigned char USER_DATA_SELECTOR = 0x20;

  static void
  install ();

private:
  // Should be consistent with selectors.s.
  static const unsigned char DESCRIPTOR_COUNT = 6;
  static const unsigned char NULL_SELECTOR = 0x00;
  static const unsigned char TSS_SELECTOR = 0x28;
  
  static gdt_ptr gp_;
  static descriptor::descriptor gdt_entry_[DESCRIPTOR_COUNT];
};

#endif /* __gdt_hpp__ */
