#ifndef __descriptor_h__
#define __descriptor_h__

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

typedef struct code_segment_descriptor code_segment_descriptor_t;
typedef struct data_segment_descriptor data_segment_descriptor_t;
typedef struct point_descriptor interrupt_descriptor_t;
typedef struct range_descriptor tss_descriptor_t;

union descriptor {
  code_segment_descriptor_t code_segment;
  data_segment_descriptor_t data_segment;
  interrupt_descriptor_t interrupt;
  tss_descriptor_t tss;
};

typedef union descriptor descriptor_t;

typedef enum {
  RING0 = 0,
  RING1 = 1,
  RING2 = 2,
  RING3 = 3,
} privilege_t;

typedef enum {
  NOT_PRESENT = 0,
  PRESENT = 1,
} present_t;

typedef enum {
  NOT_READABLE = 0,
  READABLE = 1,
} readable_t;

typedef enum {
  NOT_CONFORMING = 0,
  CONFORMING = 1,
} conforming_t;

typedef enum {
  WIDTH_16 = 0,
  WIDTH_32 = 1,
} width_t;

typedef enum {
  BYTE_GRANULARITY = 0,
  PAGE_GRANULARITY = 1,
} granularity_t;

typedef enum {
  NOT_WRITABLE = 0,
  WRITABLE = 1,
} writable_t;

typedef enum {
  EXPAND_UP = 0,
  EXPAND_DOWN = 1,
} expand_t;

descriptor_t
make_null_descriptor ();

code_segment_descriptor_t
make_code_segment_descriptor (unsigned int base,
			      unsigned int limit,
			      readable_t readable,
			      conforming_t conforming,
			      privilege_t privilege,
			      present_t present,
			      width_t width,
			      granularity_t granularity);

data_segment_descriptor_t
make_data_segment_descriptor (unsigned int base,
			      unsigned int limit,
			      writable_t writable,
			      expand_t expand,
			      privilege_t privilege,
			      present_t present,
			      width_t width,
			      granularity_t granularity);

tss_descriptor_t
make_tss_descriptor (unsigned int base,
		     unsigned int limit,
		     privilege_t privilege,
		     present_t present,
		     granularity_t granularity);

interrupt_descriptor_t
make_interrupt_gate (unsigned int offset,
		     unsigned short segment,
		     privilege_t privilege,
		     present_t present);

#endif /* __descriptor_h__ */
