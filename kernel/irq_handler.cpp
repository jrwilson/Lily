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

#define PIC_MASTER_IRQ_BASE 0
#define PIC_MASTER_IRQ_LIMIT 8
#define PIC_SLAVE_IRQ_BASE 8
#define PIC_SLAVE_IRQ_LIMIT 16

/* Remap interrupts to ISR. The assumption that they are contiguous is important. */
static const unsigned int PIC_MASTER_ISR_BASE = 32;
static const unsigned int PIC_MASTER_ISR_LIMIT = 40;
static const unsigned int PIC_SLAVE_ISR_BASE = 40;
static const unsigned int PIC_SLAVE_ISR_LIMIT = 48;

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

static unsigned char pic_master_mask_;
static unsigned char pic_slave_mask_;

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

void
irq_handler::install ()
{
  pic_master_mask_ = ~(1 << PIC_SLAVE_PIN);
  pic_slave_mask_ = 0xFF;
    
  /* Remap IRQ 0-15 to ISR 32-47. */
  io::outb (PIC_MASTER_LOW, PIC_ICW1_LOW | PIC_ICW1_NEED_ICW4);
  io::outb (PIC_SLAVE_LOW, PIC_ICW1_LOW | PIC_ICW1_NEED_ICW4);
  io::outb (PIC_MASTER_HIGH, PIC_ICW2_HIGH | PIC_MASTER_ISR_BASE);
  io::outb (PIC_SLAVE_HIGH, PIC_ICW2_HIGH | PIC_SLAVE_ISR_BASE);
  io::outb (PIC_MASTER_HIGH, PIC_ICW3_HIGH | (1 << PIC_SLAVE_PIN));
  io::outb (PIC_SLAVE_HIGH, PIC_ICW3_HIGH | PIC_ICW3_SLAVEIO_1);
  io::outb (PIC_MASTER_HIGH, PIC_ICW4_HIGH | PIC_ICW4_MODE86);
  io::outb (PIC_SLAVE_HIGH, PIC_ICW4_HIGH | PIC_ICW4_MODE86);
  /* Set the IRQ masks. */
  io::outb (PIC_MASTER_HIGH, PIC_OCW1_HIGH | pic_master_mask_);
  io::outb (PIC_SLAVE_HIGH, PIC_OCW1_HIGH | pic_slave_mask_);

  idt::set (PIC_MASTER_ISR_BASE + 0, make_interrupt_gate (irq0, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_ISR_BASE + 1, make_interrupt_gate (irq1, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_ISR_BASE + 2, make_interrupt_gate (irq2, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_ISR_BASE + 3, make_interrupt_gate (irq3, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_ISR_BASE + 4, make_interrupt_gate (irq4, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_ISR_BASE + 5, make_interrupt_gate (irq5, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_ISR_BASE + 6, make_interrupt_gate (irq6, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_MASTER_ISR_BASE + 7, make_interrupt_gate (irq7, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_ISR_BASE + 0, make_interrupt_gate (irq8, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_ISR_BASE + 1, make_interrupt_gate (irq9, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_ISR_BASE + 2, make_interrupt_gate (irq10, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_ISR_BASE + 3, make_interrupt_gate (irq11, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_ISR_BASE + 4, make_interrupt_gate (irq12, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_ISR_BASE + 5, make_interrupt_gate (irq13, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_ISR_BASE + 6, make_interrupt_gate (irq14, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (PIC_SLAVE_ISR_BASE + 7, make_interrupt_gate (irq15, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));

  // Enable interrupts.
  asm volatile ("sti");
}

void
irq_handler::enable_irq (int irq)
{
  // Change the masks on the interrupt controllers.
  if (PIC_MASTER_IRQ_BASE <= irq && irq < PIC_MASTER_IRQ_LIMIT) {
    pic_master_mask_ &= ~(1 << (irq - PIC_MASTER_IRQ_BASE));
    io::outb (PIC_MASTER_HIGH, PIC_OCW1_HIGH | pic_master_mask_); 
  }
  else if (PIC_SLAVE_IRQ_BASE <= irq && irq < PIC_SLAVE_IRQ_LIMIT) {
    pic_slave_mask_ &= ~(1 << (irq - PIC_SLAVE_IRQ_BASE));
    io::outb (PIC_SLAVE_HIGH, PIC_OCW1_HIGH | pic_slave_mask_);
  }
}

void
irq_handler::disable_irq (int irq)
{
  // Change the masks on the interrupt controllers.
  if (PIC_MASTER_IRQ_BASE <= irq && irq < PIC_MASTER_IRQ_LIMIT) {
    pic_master_mask_ |= (1 << (irq - PIC_MASTER_IRQ_BASE));
    io::outb (PIC_MASTER_HIGH, PIC_OCW1_HIGH | pic_master_mask_); 
  }
  else if (PIC_SLAVE_IRQ_BASE <= irq && irq < PIC_SLAVE_IRQ_LIMIT) {
    pic_slave_mask_ |= (1 << (irq - PIC_SLAVE_IRQ_BASE));
    io::outb (PIC_SLAVE_HIGH, PIC_OCW1_HIGH | pic_slave_mask_);
  }
}
