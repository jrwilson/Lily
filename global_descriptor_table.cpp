/*
  File
  ----
  global_descriptor_table.cpp
  
  Description
  -----------
  Global Descriptor Table (GDT) object.

  Authors:
  Justin R. Wilson
*/

#include "global_descriptor_table.hpp"

extern "C" void
gdt_flush (gdt_ptr*);

global_descriptor_table::global_descriptor_table ()
{
  gp_.limit = (sizeof (descriptor) * DESCRIPTOR_COUNT) - 1;
  gp_.base = reinterpret_cast<uint32_t> (gdt_entry_);
  
  gdt_entry_[NULL_SELECTOR / sizeof (descriptor)] = make_null_descriptor ();
  gdt_entry_[KERNEL_CODE_SELECTOR / sizeof (descriptor)].code_segment = make_code_segment_descriptor (0, 0xFFFFFFFF, descriptor_constants::READABLE, descriptor_constants::NOT_CONFORMING, descriptor_constants::RING0, descriptor_constants::PRESENT, descriptor_constants::WIDTH_32, descriptor_constants::PAGE_GRANULARITY); 
  gdt_entry_[KERNEL_DATA_SELECTOR / sizeof (descriptor)].data_segment = make_data_segment_descriptor (0, 0xFFFFFFFF, descriptor_constants::WRITABLE, descriptor_constants::EXPAND_UP, descriptor_constants::RING0, descriptor_constants::PRESENT, descriptor_constants::WIDTH_32, descriptor_constants::PAGE_GRANULARITY);
  gdt_entry_[USER_CODE_SELECTOR / sizeof (descriptor)].code_segment = make_code_segment_descriptor (0, 0xFFFFFFFF, descriptor_constants::READABLE, descriptor_constants::NOT_CONFORMING, descriptor_constants::RING3, descriptor_constants::PRESENT, descriptor_constants::WIDTH_32, descriptor_constants::PAGE_GRANULARITY);
  gdt_entry_[USER_DATA_SELECTOR / sizeof (descriptor)].data_segment = make_data_segment_descriptor (0, 0xFFFFFFFF, descriptor_constants::WRITABLE, descriptor_constants::EXPAND_UP, descriptor_constants::RING3, descriptor_constants::PRESENT, descriptor_constants::WIDTH_32, descriptor_constants::PAGE_GRANULARITY);
  
  gdt_flush (&gp_);
}
