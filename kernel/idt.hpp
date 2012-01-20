#ifndef __idt_hpp__
#define __idt_hpp__

/*
  File
  ----
  idt.hpp
  
  Description
  -----------
  Interrupt descriptor table (IDT) object.

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
  Justin R. Wilson
*/

#include "descriptor.hpp"
#include "integer_types.hpp"

class idt {
public:
  struct idt_ptr {
    uint16_t limit;
    uint32_t base;
  } __attribute__((packed));

  static void
  install ();
  
  static descriptor::interrupt_descriptor
  get (unsigned int num);
  
  static void
  set (unsigned int num,
       descriptor::interrupt_descriptor id);

private:
  static const unsigned int INTERRUPT_COUNT = 256;
  static idt::idt_ptr ip_;
  static descriptor::descriptor idt_entry_[INTERRUPT_COUNT];
};

#endif /* __idt_hpp__ */
