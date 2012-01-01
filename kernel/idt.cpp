/*
  File
  ----
  idt.cpp
  
  Description
  -----------
  Interrupt descriptor table (IDT) object.

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
  Justin R. Wilson
*/

#include "idt.hpp"
#include "kassert.hpp"

void
idt::install ()
{
  ip_.limit = (sizeof (descriptor) * INTERRUPT_COUNT) - 1;
  ip_.base = reinterpret_cast<uint32_t> (idt_entry_);
  
  for (unsigned int k = 0; k < INTERRUPT_COUNT; ++k) {
    idt_entry_[k].interrupt = make_interrupt_gate (0, 0, descriptor_constants::RING0, descriptor_constants::NOT_PRESENT);
  }

  asm ("lidt (%0)\n" : : "r"(&ip_));
}

interrupt_descriptor
idt::get (unsigned int k)
{
  kassert (k < INTERRUPT_COUNT);
  return idt_entry_[k].interrupt;
}

void
idt::set (unsigned int k,
	  interrupt_descriptor id)
{
  kassert (k < INTERRUPT_COUNT);
  kassert (idt_entry_[k].interrupt.present == descriptor_constants::NOT_PRESENT);
  idt_entry_[k].interrupt = id;
  asm ("lidt (%0)\n" : : "r"(&ip_));
}

idt::idt_ptr idt::ip_;
descriptor idt::idt_entry_[INTERRUPT_COUNT];
