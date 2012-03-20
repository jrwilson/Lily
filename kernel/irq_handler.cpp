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
#include "automaton.hpp"
#include "scheduler.hpp"

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

typedef vector<caction> subscriber_list_type;
typedef vector<subscriber_list_type> subscribers_type;
static subscribers_type subscribers_ (irq_handler::IRQ_LIMIT);

extern volatile uint32_t irq_set;

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

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND  0x43

#define PIT_WRITE_CHANNEL0 (0 << 6)
#define PIT_WRITE_CHANNEL1 (1 << 6)
#define PIT_WRITE_CHANNEL2 (2 << 6)
#define PIT_READ_CHANNEL (3 << 6)

#define PIT_LATCH (0 << 4)
#define PIT_LOW (1 << 4)
#define PIT_HIGH (2 << 4)
#define PIT_LOW_HIGH (3 << 4)

/* Interrupt on terminal count. */
#define PIT_MODE0 (0 << 1)
/* Hardware retriggerable one-shot. */
#define PIT_MODE1 (1 << 1)
/* Rate generator. */
#define PIT_MODE2 (2 << 1)
/* Square wave. */
#define PIT_MODE3 (3 << 1)
/* Software triggered strobe. */
#define PIT_MODE4 (4 << 1)
/* Hardware triggered strobe. */
#define PIT_MODE5 (5 << 1)

#define PIT_BINARY (0 << 0)
#define PIT_BCD (1 << 0)

#define PIT_READ_VALUE (1 << 4)
#define PIT_READ_STATE (2 << 4)

#define PIT_COUNTER0 (1 << 1)
#define PIT_COUNTER1 (1 << 2)
#define PIT_COUNTER2 (1 << 3)

static void
enable_irq (int irq)
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

static void
disable_irq (int irq)
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

void
irq_handler::install ()
{
  // Disable interrupts.
  asm volatile ("cli");

  // Configure the programmable interrupt controller.
  automaton::reserve_port_s (PIC_MASTER_LOW);
  automaton::reserve_port_s (PIC_MASTER_HIGH);
  automaton::reserve_port_s (PIC_SLAVE_LOW);
  automaton::reserve_port_s (PIC_SLAVE_HIGH);

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

  // Configure the programmable interval timer.
  automaton::reserve_port_s (PIT_CHANNEL0);
  automaton::reserve_port_s (PIT_CHANNEL1);
  automaton::reserve_port_s (PIT_CHANNEL2);
  automaton::reserve_port_s (PIT_COMMAND);

  /*
    http://wiki.osdev.org/Programmable_Interval_Timer

    The programmable interval timer is a frequency divider.
    Let F be the frequency of the clock that drives the PIT and let f be the frequency of the output of the PIT.
    The relationship between F and f is F / d = f where d is the frequency divisor.
    If we consider the periods T = 1 / F and t = 1 / f, then t = T * d.
    d must be an integer and when using the square wave mode must be even.

    The clock the drives the PIT has a frequency of 1.1931816666 MHz.
    Let's represent it as F = (3579395 / 3) Hz.
    If we want a kilohertz timer (f = 1000 Hz), then d = 1194 (rounded off and made even).

    The actual period of PIT output is (3 / 3579395 s) * 1194 = .00100072777662146815 s.
    Let's break down the time into three components consisting of seconds, nanoseconds, and attoseconds.
    On a clock tick, we will increment the number of attoseconds:
      a += 776621468
    The attoseconds might overflow incrementing the nano-seconds:
      while (a >= 1000000000) {
        n += 1
	a -= 1000000000
      }
    We will also increment the number of nano-seconds and account for overflow:
      n += 1000727
      while (n >= 1000000000) {
        s += 1
        n -= 1000000000
      }

    The three important constants are:
      d = 1194 (this file)
      atto_inc = 776621468 (irq.S)
      nano_inc =   1000727 (irq.S)
   */

  uint16_t period = 1194;

  /* Send the command byte. */
  io::outb (PIT_COMMAND, PIT_WRITE_CHANNEL0 | PIT_LOW_HIGH | PIT_MODE3 | PIT_BINARY);
  
  /* Send the period. */
  io::outb (PIT_CHANNEL0, period & 0xFF);
  io::outb (PIT_CHANNEL0, (period >> 8) & 0xFF);

  enable_irq (PIT_IRQ);

  // Enable interrupts.
  asm volatile ("sti");
}

void
irq_handler::subscribe (int irq,
			const caction& action)
{
  if (subscribers_[irq].empty ()) {
    enable_irq (irq);
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
    disable_irq (irq);
  }
}

void
irq_handler::process_interrupts ()
{
  // TODO:  Use atomic swap.
  asm volatile ("cli");
  uint32_t is = irq_set;
  irq_set = 0;
  asm volatile ("sti");
  
  if (is != 0) {
    uint32_t mask = 1;
    for (int irq = irq_handler::IRQ_BASE; irq != irq_handler::IRQ_LIMIT; ++irq, mask = mask << 1) {
      if ((is & mask) != 0) {
	for (subscriber_list_type::const_iterator pos = subscribers_[irq].begin ();
	     pos != subscribers_[irq].end ();
	     ++pos) {
	  scheduler::schedule_irq (*pos);
	}
      }
    }
  }
}
