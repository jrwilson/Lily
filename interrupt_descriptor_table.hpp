#ifndef __interrupt_descriptor_table_hpp__
#define __interrupt_descriptor_table_hpp__

/*
  File
  ----
  interrupt_descriptor_table.hpp
  
  Description
  -----------
  Interrupt descriptor table (IDT) object.

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
  Justin R. Wilson
*/

#include "types.hpp"
#include "kassert.hpp"
#include "descriptor.hpp"
#include "global_descriptor_table.hpp"
#include "kput.hpp"
#include "halt.hpp"
#include "io.hpp"

static const unsigned int INTERRUPT_COUNT = 256;
typedef uint16_t interrupt_number;

struct registers {
  uint32_t ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t number;
  uint32_t error;
  uint32_t eip, cs, eflags, useresp, ss;
};

struct idt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

extern "C" void
idt_flush (idt_ptr*);

class interrupt_descriptor_table {
private:
  idt_ptr ip_;
  descriptor idt_entry_[INTERRUPT_COUNT];

public:
  interrupt_descriptor_table ()
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
  get (interrupt_number k)
  {
    kassert (k < INTERRUPT_COUNT);
    return idt_entry_[k].interrupt;
  }

  void
  set (interrupt_number k,
       interrupt_descriptor id)
  {
    kassert (k < INTERRUPT_COUNT);
    kassert (idt_entry_[k].interrupt.present == descriptor_constants::NOT_PRESENT);
    idt_entry_[k].interrupt = id;
    idt_flush (&ip_);
  }
};

#endif /* __interrupt_descriptor_table_hpp__ */
