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

#include "gdt.hpp"
#include <string.h>

extern uint32_t stack_end;

struct tss_entry_t {
   uint32_t prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list.
   uint32_t esp0;       // The stack pointer to load when we change to kernel mode.
   uint32_t ss0;        // The stack segment to load when we change to kernel mode.
   uint32_t esp1;       // Unused...
   uint32_t ss1;
   uint32_t esp2;
   uint32_t ss2;
   uint32_t cr3;
   uint32_t eip;
   uint32_t eflags;
   uint32_t eax;
   uint32_t ecx;
   uint32_t edx;
   uint32_t ebx;
   uint32_t esp;
   uint32_t ebp;
   uint32_t esi;
   uint32_t edi;
   uint32_t es;         // The value to load into ES when we change to kernel mode.
   uint32_t cs;         // The value to load into CS when we change to kernel mode.
   uint32_t ss;         // The value to load into SS when we change to kernel mode.
   uint32_t ds;         // The value to load into DS when we change to kernel mode.
   uint32_t fs;         // The value to load into FS when we change to kernel mode.
   uint32_t gs;         // The value to load into GS when we change to kernel mode.
   uint32_t ldt;        // Unused...
   uint16_t trap;
   uint16_t iomap_base;
} __attribute__((packed));

static tss_entry_t tss;

extern "C" void
gdt_flush (gdt::gdt_ptr*);

void
gdt::install ()
{
  gp_.limit = (sizeof (descriptor::descriptor) * DESCRIPTOR_COUNT) - 1;
  gp_.base = reinterpret_cast<uint32_t> (gdt_entry_);
 
  gdt_entry_[NULL_SELECTOR / sizeof (descriptor::descriptor)] = descriptor::make_null_descriptor ();
  gdt_entry_[KERNEL_CODE_SELECTOR / sizeof (descriptor::descriptor)].code_segment = descriptor::make_code_segment_descriptor (0, 0xFFFFFFFF, descriptor::READABLE, descriptor::NOT_CONFORMING, descriptor::RING0, descriptor::PRESENT, descriptor::WIDTH_32, descriptor::PAGE_GRANULARITY); 
  gdt_entry_[KERNEL_DATA_SELECTOR / sizeof (descriptor::descriptor)].data_segment = descriptor::make_data_segment_descriptor (0, 0xFFFFFFFF, descriptor::WRITABLE, descriptor::EXPAND_UP, descriptor::RING0, descriptor::PRESENT, descriptor::WIDTH_32, descriptor::PAGE_GRANULARITY);
  gdt_entry_[USER_CODE_SELECTOR / sizeof (descriptor::descriptor)].code_segment = make_code_segment_descriptor (0, 0xFFFFFFFF, descriptor::READABLE, descriptor::NOT_CONFORMING, descriptor::RING3, descriptor::PRESENT, descriptor::WIDTH_32, descriptor::PAGE_GRANULARITY);
  gdt_entry_[USER_DATA_SELECTOR / sizeof (descriptor::descriptor)].data_segment = make_data_segment_descriptor (0, 0xFFFFFFFF, descriptor::WRITABLE, descriptor::EXPAND_UP, descriptor::RING3, descriptor::PRESENT, descriptor::WIDTH_32, descriptor::PAGE_GRANULARITY);
  // I am unsure about the privilege level and granularity.
  gdt_entry_[TSS_SELECTOR / sizeof (descriptor::descriptor)].tss = make_tss_descriptor (reinterpret_cast<uint32_t> (&tss), reinterpret_cast<uint32_t> (&tss) + sizeof (tss) - 1, descriptor::RING0, descriptor::PRESENT, descriptor::PAGE_GRANULARITY);

  memset (&tss, 0, sizeof (tss));

  tss.ss0 = KERNEL_DATA_SELECTOR;
  tss.esp0 = reinterpret_cast<uint32_t> (&stack_end);

  tss.cs = KERNEL_CODE_SELECTOR | descriptor::RING3;
  tss.ss = KERNEL_DATA_SELECTOR | descriptor::RING3;
  tss.ds = KERNEL_DATA_SELECTOR | descriptor::RING3;
  tss.es = KERNEL_DATA_SELECTOR | descriptor::RING3;
  tss.fs = KERNEL_DATA_SELECTOR | descriptor::RING3;
  tss.gs = KERNEL_DATA_SELECTOR | descriptor::RING3;
  
  gdt_flush (&gp_);

  asm ("ltr %%ax\n" : : "a"(TSS_SELECTOR | descriptor::RING3));
}

gdt::gdt_ptr gdt::gp_;
descriptor::descriptor gdt::gdt_entry_[DESCRIPTOR_COUNT];
