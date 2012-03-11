/*
  File
  ----
  irq_handler.hpp
  
  Description
  -----------
  Object for handling irqs.

  Authors:
  Justin R. Wilson
*/

#include "irq_handler.hpp"
#include "idt.hpp"
#include "gdt.hpp"
#include "io.hpp"
#include "kassert.hpp"
#include "scheduler.hpp"

/* Remap interrupts to ISR. The assumption that they are contiguous is important. */
static const unsigned int PIC_MASTER_BASE = 32;
static const unsigned int PIC_MASTER_LIMIT = 40;
static const unsigned int PIC_SLAVE_BASE = 40;
static const unsigned int PIC_SLAVE_LIMIT = 48;

/* Programmable interrupt controllers. */
static const unsigned char PIC_SLAVE_LOW = 0xA0;
static const unsigned char PIC_SLAVE_HIGH = 0xA1;

static const unsigned char PIC_MASTER_LOW = 0x20;
static const unsigned char PIC_MASTER_HIGH = 0x21;

/* The slave is connected to this pin on the master. */
static const unsigned char PIC_SLAVE_PIN =  2;

/* Initialization Command Words and options. */
static const unsigned char PIC_ICW1_LOW = 0x10;
static const unsigned char PIC_ICW1_NEED_ICW4 = (1 << 0);
static const unsigned char PIC_ICW1_SINGLE = (1 << 1);
static const unsigned char PIC_ICW1_CALL_INTERVAL4 = (1 << 2);
static const unsigned char PIC_ICW1_LEVEL_TRIGGER = (1 << 3);

static const unsigned char PIC_ICW2_HIGH = 0;

static const unsigned char PIC_ICW3_HIGH = 0;
static const unsigned char PIC_ICW3_SLAVEIO_0 = (1 << 0);
static const unsigned char PIC_ICW3_SLAVEIO_1 = (1 << 1);
static const unsigned char PIC_ICW3_SLAVEIO_2 = (1 << 2);

static const unsigned char PIC_ICW4_HIGH = 0;
static const unsigned char PIC_ICW4_MODE86 = (1 << 0);
static const unsigned char PIC_ICW4_AUTOEOI = (1 << 1);
static const unsigned char PIC_ICW4_NON_BUFFERED = 0x0;
static const unsigned char PIC_ICW4_BUFFERED_SLAVE = 0x8;
static const unsigned char PIC_ICW4_BUFFERED_MASTER = 0xC;
static const unsigned char PIC_ICW4_FULLY_NESTED = (1 << 4);

/* Operation Command Words and options. */
/* This OCW masks interrupts (0 -> enabled, 1 -> disabled). */
static const unsigned char PIC_OCW1_HIGH = 0;

static const unsigned char PIC_OCW2_LOW = 0;
static const unsigned char PIC_OCW2_NON_SPECIFIC_EOI = (1 << 5);
static const unsigned char PIC_OCW2_SPECIFIC_EOI = (3 << 5);
static const unsigned char PIC_OCW2_ROTATE_NON_SPECIFIC_EOI = (5 << 5);
static const unsigned char PIC_OCW2_ROTATE_AUTOMATIC_EOI_SET = (4 << 5);
static const unsigned char PIC_OCW2_ROTATE_AUTOMATIC_EOI_CLEAR = (0 << 5);
static const unsigned char PIC_OCW2_ROTATE_SPECIFIC_EOI = (7 << 5);
static const unsigned char PIC_OCW2_SET_PRIORITY = (6 << 5);
static const unsigned char PIC_OCW2_NOOP = (2 << 5);

static const unsigned char PIC_OCW3_LOW = 0x08;
static const unsigned char PIC_OCW3_READ_IR = (2 << 0);
static const unsigned char PIC_OCW3_READ_IS = (3 << 0);
static const unsigned char PIC_OCW3_POLL = (1 << 2);
static const unsigned char PIC_OCW3_RESET_SPECIAL_MASK = (2 << 5);
static const unsigned char PIC_OCW3_SET_SPECIAL_MASK = (3 << 5);

extern "C" void irq0 ();
extern "C" void irq1 ();
extern "C" void irq2 ();
extern "C" void irq3 ();
extern "C" void irq4 ();
extern "C" void irq5 ();
extern "C" void irq6 ();
extern "C" void irq7 ();
extern "C" void irq8 ();
extern "C" void irq9 ();
extern "C" void irq10 ();
extern "C" void irq11 ();
extern "C" void irq12 ();
extern "C" void irq13 ();
extern "C" void irq14 ();
extern "C" void irq15 ();

static uint8_t pic_master_mask_;
static uint8_t pic_slave_mask_;

typedef vector<caction> subscriber_list_type;
typedef vector<subscriber_list_type> subscribers_type;
static subscribers_type subscribers_ (irq_handler::MAX_IRQ + 1);

void
irq_handler::install ()
{
  pic_master_mask_ = ~(1 << PIC_SLAVE_PIN);
  pic_slave_mask_ = 0xFF;

  /* Remap IRQ 0-15 to ISR 32-47. */
  io::outb (PIC_MASTER_LOW, PIC_ICW1_LOW | PIC_ICW1_NEED_ICW4);
  io::outb (PIC_SLAVE_LOW, PIC_ICW1_LOW | PIC_ICW1_NEED_ICW4);
  io::outb (PIC_MASTER_HIGH, PIC_ICW2_HIGH | PIC_MASTER_BASE);
  io::outb (PIC_SLAVE_HIGH, PIC_ICW2_HIGH | PIC_SLAVE_BASE);
  io::outb (PIC_MASTER_HIGH, PIC_ICW3_HIGH | (1 << PIC_SLAVE_PIN));
  io::outb (PIC_SLAVE_HIGH, PIC_ICW3_HIGH | PIC_ICW3_SLAVEIO_1);
  io::outb (PIC_MASTER_HIGH, PIC_ICW4_HIGH | PIC_ICW4_MODE86);
  io::outb (PIC_SLAVE_HIGH, PIC_ICW4_HIGH | PIC_ICW4_MODE86);
  /* Set the interrupt masks. */
  io::outb (PIC_MASTER_HIGH, PIC_OCW1_HIGH | pic_master_mask_);
  io::outb (PIC_SLAVE_HIGH, PIC_OCW1_HIGH | pic_slave_mask_);

  idt::set (PIC_MASTER_BASE + 0, make_interrupt_gate (irq0, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_BASE + 1, make_interrupt_gate (irq1, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_BASE + 2, make_interrupt_gate (irq2, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_BASE + 3, make_interrupt_gate (irq3, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_BASE + 4, make_interrupt_gate (irq4, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_BASE + 5, make_interrupt_gate (irq5, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_BASE + 6, make_interrupt_gate (irq6, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_BASE + 7, make_interrupt_gate (irq7, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_BASE + 0, make_interrupt_gate (irq8, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_BASE + 1, make_interrupt_gate (irq9, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_BASE + 2, make_interrupt_gate (irq10, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_BASE + 3, make_interrupt_gate (irq11, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_BASE + 4, make_interrupt_gate (irq12, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_BASE + 5, make_interrupt_gate (irq13, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_BASE + 6, make_interrupt_gate (irq14, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_BASE + 7, make_interrupt_gate (irq15, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
}

void
irq_handler::subscribe (int irq,
			const caction& action)
{
  if (subscribers_[irq].empty ()) {
    unsigned int isr = irq + PIC_MASTER_BASE;
    // Change the masks on the interrupt controllers.
    if (PIC_MASTER_BASE <= isr && isr < PIC_MASTER_LIMIT) {
      pic_master_mask_ &= ~(1 << (isr - PIC_MASTER_BASE));
      io::outb (PIC_MASTER_HIGH, PIC_OCW1_HIGH | pic_master_mask_); 
    }
    else if (PIC_SLAVE_BASE <= isr && isr < PIC_SLAVE_LIMIT) {
      pic_slave_mask_ &= ~(1 << (isr - PIC_SLAVE_BASE));
      io::outb (PIC_SLAVE_HIGH, PIC_OCW1_HIGH | pic_slave_mask_);
    }
  }

  subscribers_[irq].push_back (action);
}

void
irq_handler::unsubscribe (int irq,
			  const caction& action)
{
  subscriber_list_type::iterator pos = find (subscribers_[irq].begin (), subscribers_[irq].end (), action);
  kassert (pos != subscribers_[irq].end ());
  subscribers_[irq].erase (pos);

  if (subscribers_[irq].empty ()) {
    unsigned int isr = irq + PIC_MASTER_BASE;
    // Change the masks on the interrupt controllers.
    if (PIC_MASTER_BASE <= isr && isr < PIC_MASTER_LIMIT) {
      pic_master_mask_ |= (1 << (isr - PIC_MASTER_BASE));
      io::outb (PIC_MASTER_HIGH, PIC_OCW1_HIGH | pic_master_mask_); 
    }
    else if (PIC_SLAVE_BASE <= isr && isr < PIC_SLAVE_LIMIT) {
      pic_slave_mask_ |= (1 << (isr - PIC_SLAVE_BASE));
      io::outb (PIC_SLAVE_HIGH, PIC_OCW1_HIGH | pic_slave_mask_);
    }
  }
}

extern "C" void
irq_dispatch (volatile registers regs)
{
  const int irq = regs.number - PIC_MASTER_BASE;
  for (subscriber_list_type::const_iterator pos = subscribers_[irq].begin ();
       pos != subscribers_[irq].end ();
       ++pos) {
    scheduler::schedule (*pos);
  }

  /* Send end-of-interrupt. */
  if (PIC_SLAVE_BASE <= regs.number && regs.number < PIC_SLAVE_LIMIT) {
    io::outb (PIC_SLAVE_LOW, PIC_OCW2_LOW | PIC_OCW2_NON_SPECIFIC_EOI);
  }
  io::outb (PIC_MASTER_LOW, PIC_OCW2_LOW | PIC_OCW2_NON_SPECIFIC_EOI);
}
