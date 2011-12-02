/*
  File
  ----
  interrupt_descriptor_table.cpp
  
  Description
  -----------
  Interrupt descriptor table (IDT) object.

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
  Justin R. Wilson
*/

#include "interrupt_descriptor_table.hpp"
#include "kassert.hpp"

struct idt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

static const unsigned int INTERRUPT_COUNT = 256;
static idt_ptr ip_;
static descriptor idt_entry_[INTERRUPT_COUNT];

extern "C" void
idt_flush (idt_ptr*);

void
interrupt_descriptor_table::install ()
{
  interrupt_number k;
  
  ip_.limit = (sizeof (descriptor) * INTERRUPT_COUNT) - 1;
  ip_.base = reinterpret_cast<uint32_t> (idt_entry_);
  
  for (k = 0; k < INTERRUPT_COUNT; ++k) {
    idt_entry_[k].interrupt = make_interrupt_gate (0, 0, descriptor_constants::RING0, descriptor_constants::NOT_PRESENT);
  }
  
  idt_flush (&ip_);
}

interrupt_descriptor
interrupt_descriptor_table::get (interrupt_number k)
{
  kassert (k < INTERRUPT_COUNT);
  return idt_entry_[k].interrupt;
}

void
interrupt_descriptor_table::set (interrupt_number k,
				 interrupt_descriptor id)
{
  kassert (k < INTERRUPT_COUNT);
  kassert (idt_entry_[k].interrupt.present == descriptor_constants::NOT_PRESENT);
  idt_entry_[k].interrupt = id;
  idt_flush (&ip_);
}
