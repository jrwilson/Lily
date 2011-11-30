#ifndef __descriptor_hpp__
#define __descriptor_hpp__

/*
  File
  ----
  descriptor.hpp
  
  Description
  -----------
  Types and functions for managing x86 descriptors.

  Authors:
  Justin R. Wilson
*/

/* For code segments. */
struct code_segment_descriptor {
  unsigned int limit_low : 16;
  unsigned int base_low : 16;
  unsigned int base_middle : 8;
  unsigned int accessed : 1;
  unsigned int readable : 1;
  unsigned int conforming : 1;
  unsigned int code : 1; /* Set to 1. */
  unsigned int memory_segment : 1; /* Set to 1. */
  unsigned int dpl : 2;
  unsigned int present : 1;
  unsigned int limit_high : 4;
  unsigned int available : 1;
  unsigned int zero : 1;
  unsigned int width : 1;
  unsigned int granularity : 1;
  unsigned int base_high : 8;
} __attribute__ ((packed));

/* For data segments. */
struct data_segment_descriptor {
  unsigned int limit_low : 16;
  unsigned int base_low : 16;
  unsigned int base_middle : 8;
  unsigned int accessed : 1;
  unsigned int writable : 1;
  unsigned int expand_down : 1;
  unsigned int code : 1; /* Set to 0. */
  unsigned int memory_segment : 1; /* Set to 1. */
  unsigned int dpl : 2;
  unsigned int present : 1;
  unsigned int limit_high : 4;
  unsigned int available : 1;
  unsigned int zero : 1;
  unsigned int width : 1;
  unsigned int granularity : 1;
  unsigned int base_high : 8;
} __attribute__ ((packed));

/* For local descriptor tables (LDT) and task state segments (TSS). */
struct range_descriptor {
  unsigned int limit_low : 16;
  unsigned int base_low : 16;
  unsigned int base_middle : 8;
  unsigned int type : 5;
  unsigned int dpl : 2;
  unsigned int present : 1;
  unsigned int limit_high : 4;
  unsigned int available : 1;
  unsigned int zero : 1;
  unsigned int width : 1;
  unsigned int granularity : 1;
  unsigned int base_high : 8;
} __attribute__ ((packed));

/* For call gates, interrupt gates, trap gates, and task gates. */
struct point_descriptor {
  unsigned int offset_low : 16;
  unsigned int segment : 16;
  unsigned int parameter_count : 5;
  unsigned int zero : 3;
  unsigned int type : 5;
  unsigned int dpl : 2;
  unsigned int present : 1;
  unsigned int offset_high : 16;
} __attribute__ ((packed));

typedef point_descriptor interrupt_descriptor;
typedef range_descriptor tss_descriptor;

union descriptor {
  code_segment_descriptor code_segment;
  data_segment_descriptor data_segment;
  interrupt_descriptor interrupt;
  tss_descriptor tss;
};

enum privilege_t {
  RING0 = 0,
  RING1 = 1,
  RING2 = 2,
  RING3 = 3,
};

enum present_t {
  NOT_PRESENT = 0,
  PRESENT = 1,
};

enum readable_t {
  NOT_READABLE = 0,
  READABLE = 1,
};

enum conforming_t {
  NOT_CONFORMING = 0,
  CONFORMING = 1,
};

enum width_t {
  WIDTH_16 = 0,
  WIDTH_32 = 1,
};

enum granularity_t {
  BYTE_GRANULARITY = 0,
  PAGE_GRANULARITY = 1,
};

enum writable_t {
  NOT_WRITABLE = 0,
  WRITABLE = 1,
};

enum expand_t {
  EXPAND_UP = 0,
  EXPAND_DOWN = 1,
};

enum segment_type_t {
  INTERRUPT_GATE = 0x0E,
  AVAILABLE_TSS = 0x09,
};

inline descriptor
make_null_descriptor ()
{
  descriptor retval;
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

inline code_segment_descriptor
make_code_segment_descriptor (unsigned int base,
			      unsigned int limit,
			      readable_t readable,
			      conforming_t conforming,
			      privilege_t privilege,
			      present_t present,
			      width_t width,
			      granularity_t granularity)
{
  code_segment_descriptor retval;

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

inline data_segment_descriptor
make_data_segment_descriptor (unsigned int base,
			      unsigned int limit,
			      writable_t writable,
			      expand_t expand,
			      privilege_t privilege,
			      present_t present,
			      width_t width,
			      granularity_t granularity)
{
  data_segment_descriptor retval;

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

inline tss_descriptor
make_tss_descriptor (unsigned int base,
		     unsigned int limit,
		     privilege_t privilege,
		     present_t present,
		     granularity_t granularity)
{
  tss_descriptor retval;

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

typedef void (*interrupt_handler) ();

inline interrupt_descriptor
make_interrupt_gate (interrupt_handler handler,
		     unsigned short segment,
		     privilege_t privilege,
		     present_t present)
{
  interrupt_descriptor retval;
  unsigned int offset = reinterpret_cast<unsigned int> (handler);

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

#endif /* __descriptor_hpp__ */
