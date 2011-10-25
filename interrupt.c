/*
  File
  ----
  interrupt.c
  
  Description
  -----------
  Functions for managing interrupts.

  Authors
  -------
  http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
  Justin R. Wilson
*/

#include "interrupt.h"
#include "memory.h"
#include "kput.h"
#include "io.h"
#include "halt.h"
#include "kassert.h"

#define INTERRUPT_COUNT 256

/* Macros for the flag byte of interrupt descriptors. */

/* Is the interrupt present? */
#define NOT_PRESENT (0 << 7)
#define PRESENT (1 << 7)

/* What is the interrupt's privilege level? */
#define RING0 (0 << 5)
#define RING1 (1 << 5)
#define RING2 (2 << 5)
#define RING3 (3 << 5)

/* Does the segment describe memory or something else in the system? */
#define SYSTEM (0 << 4)
#define MEMORY (1 << 4)

/* Clears the IF flag. */
#define INTERRUPT_GATE_32 0xE
/* Doesn't clear the IF flag. */
#define TRAP_GATE_32 0xF

/* Remap interrupts to ISR. */
#define PIC_MASTER_BASE 32
#define PIC_MASTER_LIMIT 40

#define PIC_SLAVE_LOW 0xA0
#define PIC_SLAVE_HIGH 0xA1

/* Programmable interrupt controller. */
#define PIC_MASTER_LOW 0x20
#define PIC_MASTER_HIGH 0x21

#define PIC_SLAVE_BASE 40
#define PIC_SLAVE_LIMIT 48

#define PIC_SLAVE_PIN 2

/* Initialization Command Words and options. */
#define PIC_ICW1_LOW 0x10
#define PIC_ICW1_NEED_ICW4 (1 << 0)
#define PIC_ICW1_SINGLE (1 << 1)
#define PIC_ICW1_CALL_INTERVAL4 (1 << 2)
#define PIC_ICW1_LEVEL_TRIGGER (1 << 3)

#define PIC_ICW2_HIGH 0

#define PIC_ICW3_HIGH 0
#define PIC_ICW3_SLAVEIO_0 (1 << 0)
#define PIC_ICW3_SLAVEIO_1 (1 << 1)
#define PIC_ICW3_SLAVEIO_2 (1 << 2)

#define PIC_ICW4_HIGH 0
#define PIC_ICW4_MODE86 (1 << 0)
#define PIC_ICW4_AUTOEOI (1 << 1)
#define PIC_ICW4_NON_BUFFERED 0x0
#define PIC_ICW4_BUFFERED_SLAVE 0x8
#define PIC_ICW4_BUFFERED_MASTER 0xC
#define PIC_ICW4_FULLY_NESTED (1 << 4)

/* Operation Command Words and options. */
/* This OCW masks interrupts (0 -> enabled, 1 -> disabled). */
#define PIC_OCW1_HIGH 0

#define PIC_OCW2_LOW 0
#define PIC_OCW2_NON_SPECIFIC_EOI (1 << 5)
#define PIC_OCW2_SPECIFIC_EOI (3 << 5)
#define PIC_OCW2_ROTATE_NON_SPECIFIC_EOI (5 << 5)
#define PIC_OCW2_ROTATE_AUTOMATIC_EOI_SET (4 << 5)
#define PIC_OCW2_ROTATE_AUTOMATIC_EOI_CLEAR (0 << 5)
#define PIC_OCW2_ROTATE_SPECIFIC_EOI (7 << 5)
#define PIC_OCW2_SET_PRIORITY (6 << 5)
#define PIC_OCW2_NOOP (2 << 5)

#define PIC_OCW3_LOW 0x08
#define PIC_OCW3_READ_IR (2 << 0)
#define PIC_OCW3_READ_IS (3 << 0)
#define PIC_OCW3_POLL (1 << 2)
#define PIC_OCW3_RESET_SPECIAL_MASK (2 << 5)
#define PIC_OCW3_SET_SPECIAL_MASK (3 << 5)

struct idt_entry
{
  unsigned short base_low;
  unsigned short segment;
  unsigned char zero;
  unsigned char flags;
  unsigned short base_high;
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

struct idt_ptr
{
  unsigned short limit;
  idt_entry_t* base;
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

static idt_entry_t idt[INTERRUPT_COUNT];
static idt_ptr_t ip;
static interrupt_handler_t interrupt_handlers[INTERRUPT_COUNT];

extern void exception0 ();
extern void exception1 ();
extern void exception2 ();
extern void exception3 ();
extern void exception4 ();
extern void exception5 ();
extern void exception6 ();
extern void exception7 ();
extern void exception8 ();
extern void exception9 ();
extern void exception10 ();
extern void exception11 ();
extern void exception12 ();
extern void exception13 ();
extern void exception14 ();
extern void exception15 ();
extern void exception16 ();
extern void exception17 ();
extern void exception18 ();
extern void exception19 ();
extern void exception20 ();
extern void exception21 ();
extern void exception22 ();
extern void exception23 ();
extern void exception24 ();
extern void exception25 ();
extern void exception26 ();
extern void exception27 ();
extern void exception28 ();
extern void exception29 ();
extern void exception30 ();
extern void exception31 ();

extern void irq0 ();
extern void irq1 ();
extern void irq2 ();
extern void irq3 ();
extern void irq4 ();
extern void irq5 ();
extern void irq6 ();
extern void irq7 ();
extern void irq8 ();
extern void irq9 ();
extern void irq10 ();
extern void irq11 ();
extern void irq12 ();
extern void irq13 ();
extern void irq14 ();
extern void irq15 ();

extern void
idt_flush (idt_ptr_t*);

static void
idt_set_gate (unsigned char num,
	      unsigned int base,
	      unsigned short segment,
	      unsigned char flags)
{
  idt[num].base_low = base & 0xFFFF;
  idt[num].base_high = (base >> 16) & 0xFFFF;

  idt[num].segment = segment;
  idt[num].zero = 0;
  idt[num].flags = flags;
}

static void
default_handler (registers_t* regs)
{
  kputs ("Unhandled interrupt!\n");
  kputs ("Interrupt: "); kputuix (regs->number); kputs (" Code: " ); kputuix (regs->error); kputs ("\n");
  
  kputs ("CS: "); kputuix (regs->cs); kputs (" EIP: "); kputuix (regs->eip); kputs (" EFLAGS: "); kputuix (regs->eflags); kputs ("\n");
  kputs ("SS: "); kputuix (regs->ss); kputs (" ESP: "); kputuix (regs->useresp); kputs (" DS:"); kputuix (regs->ds); kputs ("\n");
  
  kputs ("EAX: "); kputuix (regs->eax); kputs (" EBX: "); kputuix (regs->ebx); kputs (" ECX: "); kputuix (regs->ecx); kputs (" EDX: "); kputuix (regs->edx); kputs ("\n");
  kputs ("ESP: "); kputuix (regs->esp); kputs (" EBP: "); kputuix (regs->ebp); kputs (" ESI: "); kputuix (regs->esi); kputs (" EDI: "); kputuix (regs->edi); kputs ("\n");

  halt ();
}

void
install_idt ()
{
  int k;

  for (k = 0; k < INTERRUPT_COUNT; ++k) {
    set_interrupt_handler (k, default_handler);
  }

  ip.limit = (sizeof (idt_entry_t) * INTERRUPT_COUNT) - 1;
  ip.base = idt;

  for (k = 0; k < INTERRUPT_COUNT; ++k) {
    idt_set_gate (k, 0, 0, 0);
  }

  idt_set_gate (0, (unsigned int)exception0, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (1, (unsigned int)exception1, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (2, (unsigned int)exception2, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (3, (unsigned int)exception3, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (4, (unsigned int)exception4, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (5, (unsigned int)exception5, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (6, (unsigned int)exception6, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (7, (unsigned int)exception7, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (8, (unsigned int)exception8, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (9, (unsigned int)exception9, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (10, (unsigned int)exception10, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (11, (unsigned int)exception11, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (12, (unsigned int)exception12, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (13, (unsigned int)exception13, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (14, (unsigned int)exception14, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (15, (unsigned int)exception15, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (16, (unsigned int)exception16, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (17, (unsigned int)exception17, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (18, (unsigned int)exception18, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (19, (unsigned int)exception19, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (20, (unsigned int)exception20, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (21, (unsigned int)exception21, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (22, (unsigned int)exception22, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (23, (unsigned int)exception23, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (24, (unsigned int)exception24, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (25, (unsigned int)exception25, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (26, (unsigned int)exception26, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (27, (unsigned int)exception27, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (28, (unsigned int)exception28, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (29, (unsigned int)exception29, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (30, (unsigned int)exception30, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (31, (unsigned int)exception31, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);

  /* Remap IRQ 0-15 to ISR 32-47. */
  outb (PIC_MASTER_LOW, PIC_ICW1_LOW | PIC_ICW1_NEED_ICW4);
  outb (PIC_SLAVE_LOW, PIC_ICW1_LOW | PIC_ICW1_NEED_ICW4);
  outb (PIC_MASTER_HIGH, PIC_ICW2_HIGH | PIC_MASTER_BASE);
  outb (PIC_SLAVE_HIGH, PIC_ICW2_HIGH | PIC_SLAVE_BASE);
  outb (PIC_MASTER_HIGH, PIC_ICW3_HIGH | (1 << PIC_SLAVE_PIN));
  outb (PIC_SLAVE_HIGH, PIC_ICW3_HIGH | PIC_ICW3_SLAVEIO_1);
  outb (PIC_MASTER_HIGH, PIC_ICW4_HIGH | PIC_ICW4_MODE86);
  outb (PIC_SLAVE_HIGH, PIC_ICW4_HIGH | PIC_ICW4_MODE86);
  outb (PIC_MASTER_HIGH, 0x0);
  outb (PIC_SLAVE_HIGH, 0x0);

  idt_set_gate (PIC_MASTER_BASE + 0, (unsigned int)irq0, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 1, (unsigned int)irq1, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 2, (unsigned int)irq2, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 3, (unsigned int)irq3, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 4, (unsigned int)irq4, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 5, (unsigned int)irq5, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 6, (unsigned int)irq6, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 7, (unsigned int)irq7, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 0, (unsigned int)irq8, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 1, (unsigned int)irq9, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 2, (unsigned int)irq10, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 3, (unsigned int)irq11, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 4, (unsigned int)irq12, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 5, (unsigned int)irq13, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 6, (unsigned int)irq14, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 7, (unsigned int)irq15, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);

  idt_flush (&ip);
}

void
exception_handler (registers_t regs)
{
  if (interrupt_handlers[regs.number]) {
    interrupt_handlers[regs.number] (&regs);
  }
  else {
    default_handler (&regs);
  }
}

void
irq_handler (registers_t regs)
{
  /* STABLE:  interrupt_handlers[regs.number] != 0.
     This is established in install_idt and maintained by get_interrupt_handler and set_interrupt_handler. */
  interrupt_handlers[regs.number] (&regs);

  /* Send end-of-interrupt. */
  if (PIC_SLAVE_BASE <= regs.number && regs.number < PIC_SLAVE_LIMIT) {
    outb (PIC_SLAVE_LOW, PIC_OCW2_LOW | PIC_OCW2_NON_SPECIFIC_EOI);
  }
  outb (PIC_MASTER_LOW, PIC_OCW2_LOW | PIC_OCW2_NON_SPECIFIC_EOI);
}

void
enable_interrupts ()
{
  __asm__ __volatile__ ("sti");
}

void
disable_interrupts ()
{
  __asm__ __volatile__ ("cli");
}

interrupt_handler_t
get_interrupt_handler (unsigned int num)
{
  kassert (num < INTERRUPT_COUNT);
  return interrupt_handlers[num];
}

void
set_interrupt_handler (unsigned int num,
		       interrupt_handler_t handler)
{
  kassert (num < INTERRUPT_COUNT);
  if (handler != 0) {
    interrupt_handlers[num] = handler;
  }
  else {
    interrupt_handlers[num] = default_handler;
  }
}
