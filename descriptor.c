/*
  File
  ----
  descriptor.h
  
  Description
  -----------
  Types and functions for managing x86 descriptors.

  Authors:
  Justin R. Wilson
*/

#include "descriptor.h"
#include "kassert.h"

typedef enum {
  INTERRUPT_GATE = 0x0E,
  AVAILABLE_TSS = 0x09,
} segment_type_t;

descriptor_t
make_null_descriptor ()
{
  descriptor_t retval;
  retval.interrupt.offset_low = 0;
  retval.interrupt.offset_high = 0;
  retval.interrupt.segment = 0;
  retval.interrupt.parameter_count = 0;
  retval.interrupt.zero = 0;
  retval.interrupt.type = 0;
  retval.interrupt.dpl = 0;
  retval.interrupt.present = 0;
  return retval;
}

code_segment_descriptor_t
make_code_segment_descriptor (unsigned int base,
			      unsigned int limit,
			      readable_t readable,
			      conforming_t conforming,
			      privilege_t privilege,
			      present_t present,
			      width_t width,
			      granularity_t granularity)
{
  code_segment_descriptor_t retval;

  retval.base_low = (base & 0xFFFF);
  retval.base_middle = (base >> 16) & 0xFF;
  retval.base_high = (base >> 24) & 0xFF;

  if (granularity == PAGE_GRANULARITY) {
    limit >>= 12;
  }

  retval.limit_low = (limit & 0xFFFF);
  retval.limit_high = (limit >> 16) & 0xF;

  retval.accessed = 0;
  retval.readable = readable;
  retval.conforming = conforming;
  retval.code = 1;
  retval.memory_segment = 1;
  retval.dpl = privilege;
  retval.present = present;
  retval.available = 0;
  retval.zero = 0;
  retval.width = width;
  retval.granularity = granularity;

  return retval;
}

data_segment_descriptor_t
make_data_segment_descriptor (unsigned int base,
			      unsigned int limit,
			      writable_t writable,
			      expand_t expand,
			      privilege_t privilege,
			      present_t present,
			      width_t width,
			      granularity_t granularity)
{
  data_segment_descriptor_t retval;

  retval.base_low = (base & 0xFFFF);
  retval.base_middle = (base >> 16) & 0xFF;
  retval.base_high = (base >> 24) & 0xFF;

  if (granularity == PAGE_GRANULARITY) {
    limit >>= 12;
  }

  retval.limit_low = (limit & 0xFFFF);
  retval.limit_high = (limit >> 16) & 0xF;

  retval.accessed = 0;
  retval.writable = writable;
  retval.expand_down = expand;
  retval.code = 0;
  retval.memory_segment = 1;
  retval.dpl = privilege;
  retval.present = present;
  retval.available = 0;
  retval.zero = 0;
  retval.width = width;
  retval.granularity = granularity;

  return retval;
}

tss_descriptor_t
make_tss_descriptor (unsigned int base,
		     unsigned int limit,
		     privilege_t privilege,
		     present_t present,
		     granularity_t granularity)
{
  tss_descriptor_t retval;

  retval.base_low = (base & 0xFFFF);
  retval.base_middle = (base >> 16) & 0xFF;
  retval.base_high = (base >> 24) & 0xFF;

  if (granularity == PAGE_GRANULARITY) {
    limit >>= 12;
  }

  retval.limit_low = (limit & 0xFFFF);
  retval.limit_high = (limit >> 16) & 0xF;

  retval.type = AVAILABLE_TSS;
  retval.dpl = privilege;
  retval.present = present;
  retval.available = 0;
  retval.zero = 0;
  retval.width = 0;
  retval.granularity = granularity;

  return retval;
}

interrupt_descriptor_t
make_interrupt_gate (unsigned int offset,
		     unsigned short segment,
		     privilege_t privilege,
		     present_t present)
{
  interrupt_descriptor_t retval;

  retval.offset_low = offset & 0xFFFF;
  retval.offset_high = (offset >> 16) & 0xFFFF;
  retval.segment = segment;
  retval.parameter_count = 0;
  retval.zero = 0;
  retval.type = INTERRUPT_GATE;
  retval.dpl = privilege;
  retval.present = present;

  return retval;
}
