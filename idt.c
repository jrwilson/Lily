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

#include "idt.h"
#include "kput.h"
#include "io.h"
#include "halt.h"
#include "kassert.h"
#include "descriptor.h"
#include "gdt.h"

#define INTERRUPT_COUNT 256

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

/* The slave is connected to this pin on the master. */
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

struct idt_ptr
{
  unsigned short limit;
  unsigned int base;
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

static descriptor_t idt_entry[INTERRUPT_COUNT];
static idt_ptr_t ip;
static interrupt_handler_t interrupt_handler[INTERRUPT_COUNT];

static unsigned char pic_master_mask = ~(1 << PIC_SLAVE_PIN);
static unsigned char pic_slave_mask = 0xFF;

extern "C" void exception0 ();
extern "C" void exception1 ();
extern "C" void exception2 ();
extern "C" void exception3 ();
extern "C" void exception4 ();
extern "C" void exception5 ();
extern "C" void exception6 ();
extern "C" void exception7 ();
extern "C" void exception8 ();
extern "C" void exception9 ();
extern "C" void exception10 ();
extern "C" void exception11 ();
extern "C" void exception12 ();
extern "C" void exception13 ();
extern "C" void exception14 ();
extern "C" void exception15 ();
extern "C" void exception16 ();
extern "C" void exception17 ();
extern "C" void exception18 ();
extern "C" void exception19 ();
extern "C" void exception20 ();
extern "C" void exception21 ();
extern "C" void exception22 ();
extern "C" void exception23 ();
extern "C" void exception24 ();
extern "C" void exception25 ();
extern "C" void exception26 ();
extern "C" void exception27 ();
extern "C" void exception28 ();
extern "C" void exception29 ();
extern "C" void exception30 ();
extern "C" void exception31 ();

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

extern "C" void trap0 ();

extern "C" void
idt_flush (idt_ptr_t*);

static void
default_handler (registers_t* regs)
{
  kputs ("Unhandled interrupt!\n");
  kputs ("Interrupt: "); kputx32 (regs->number); kputs (" Code: " ); kputx32 (regs->error); kputs ("\n");
  
  kputs ("CS: "); kputx32 (regs->cs); kputs (" EIP: "); kputx32 (regs->eip); kputs (" EFLAGS: "); kputx32 (regs->eflags); kputs ("\n");
  kputs ("SS: "); kputx32 (regs->ss); kputs (" ESP: "); kputx32 (regs->useresp); kputs (" DS:"); kputx32 (regs->ds); kputs ("\n");
  
  kputs ("EAX: "); kputx32 (regs->eax); kputs (" EBX: "); kputx32 (regs->ebx); kputs (" ECX: "); kputx32 (regs->ecx); kputs (" EDX: "); kputx32 (regs->edx); kputs ("\n");
  kputs ("ESP: "); kputx32 (regs->esp); kputs (" EBP: "); kputx32 (regs->ebp); kputs (" ESI: "); kputx32 (regs->esi); kputs (" EDI: "); kputx32 (regs->edi); kputs ("\n");

  halt ();
}

void
idt_initialize ()
{
  int k;

  for (k = 0; k < INTERRUPT_COUNT; ++k) {
    /* Do not use set_interrupt_handler because it changes the interrupt controller masks. */
    interrupt_handler[k] = default_handler;
  }

  ip.limit = (sizeof (descriptor_t) * INTERRUPT_COUNT) - 1;
  ip.base = (unsigned int)idt_entry;

  for (k = 0; k < INTERRUPT_COUNT; ++k) {
    idt_entry[k].interrupt = make_interrupt_gate (0, 0, RING0, NOT_PRESENT);
  }

  idt_entry[0].interrupt = make_interrupt_gate ((unsigned int)exception0, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[1].interrupt = make_interrupt_gate ((unsigned int)exception1, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[2].interrupt = make_interrupt_gate ((unsigned int)exception2, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[3].interrupt = make_interrupt_gate ((unsigned int)exception3, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[4].interrupt = make_interrupt_gate ((unsigned int)exception4, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[5].interrupt = make_interrupt_gate ((unsigned int)exception5, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[6].interrupt = make_interrupt_gate ((unsigned int)exception6, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[7].interrupt = make_interrupt_gate ((unsigned int)exception7, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[8].interrupt = make_interrupt_gate ((unsigned int)exception8, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[9].interrupt = make_interrupt_gate ((unsigned int)exception9, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[10].interrupt = make_interrupt_gate ((unsigned int)exception10, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[11].interrupt = make_interrupt_gate ((unsigned int)exception11, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[12].interrupt = make_interrupt_gate ((unsigned int)exception12, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[13].interrupt = make_interrupt_gate ((unsigned int)exception13, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[14].interrupt = make_interrupt_gate ((unsigned int)exception14, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[15].interrupt = make_interrupt_gate ((unsigned int)exception15, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[16].interrupt = make_interrupt_gate ((unsigned int)exception16, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[17].interrupt = make_interrupt_gate ((unsigned int)exception17, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[18].interrupt = make_interrupt_gate ((unsigned int)exception18, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[19].interrupt = make_interrupt_gate ((unsigned int)exception19, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[20].interrupt = make_interrupt_gate ((unsigned int)exception20, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[21].interrupt = make_interrupt_gate ((unsigned int)exception21, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[22].interrupt = make_interrupt_gate ((unsigned int)exception22, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[23].interrupt = make_interrupt_gate ((unsigned int)exception23, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[24].interrupt = make_interrupt_gate ((unsigned int)exception24, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[25].interrupt = make_interrupt_gate ((unsigned int)exception25, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[26].interrupt = make_interrupt_gate ((unsigned int)exception26, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[27].interrupt = make_interrupt_gate ((unsigned int)exception27, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[28].interrupt = make_interrupt_gate ((unsigned int)exception28, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[29].interrupt = make_interrupt_gate ((unsigned int)exception29, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[30].interrupt = make_interrupt_gate ((unsigned int)exception30, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[31].interrupt = make_interrupt_gate ((unsigned int)exception31, KERNEL_CODE_SELECTOR, RING0, PRESENT);

  /* Remap IRQ 0-15 to ISR 32-47. */
  outb (PIC_MASTER_LOW, PIC_ICW1_LOW | PIC_ICW1_NEED_ICW4);
  outb (PIC_SLAVE_LOW, PIC_ICW1_LOW | PIC_ICW1_NEED_ICW4);
  outb (PIC_MASTER_HIGH, PIC_ICW2_HIGH | PIC_MASTER_BASE);
  outb (PIC_SLAVE_HIGH, PIC_ICW2_HIGH | PIC_SLAVE_BASE);
  outb (PIC_MASTER_HIGH, PIC_ICW3_HIGH | (1 << PIC_SLAVE_PIN));
  outb (PIC_SLAVE_HIGH, PIC_ICW3_HIGH | PIC_ICW3_SLAVEIO_1);
  outb (PIC_MASTER_HIGH, PIC_ICW4_HIGH | PIC_ICW4_MODE86);
  outb (PIC_SLAVE_HIGH, PIC_ICW4_HIGH | PIC_ICW4_MODE86);
  /* Set the interrupt masks. */
  outb (PIC_MASTER_HIGH, PIC_OCW1_HIGH | pic_master_mask);
  outb (PIC_SLAVE_HIGH, PIC_OCW1_HIGH | pic_slave_mask);

  idt_entry[PIC_MASTER_BASE + 0].interrupt = make_interrupt_gate ((unsigned int)irq0, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_MASTER_BASE + 1].interrupt = make_interrupt_gate ((unsigned int)irq1, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_MASTER_BASE + 2].interrupt = make_interrupt_gate ((unsigned int)irq2, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_MASTER_BASE + 3].interrupt = make_interrupt_gate ((unsigned int)irq3, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_MASTER_BASE + 4].interrupt = make_interrupt_gate ((unsigned int)irq4, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_MASTER_BASE + 5].interrupt = make_interrupt_gate ((unsigned int)irq5, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_MASTER_BASE + 6].interrupt = make_interrupt_gate ((unsigned int)irq6, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_MASTER_BASE + 7].interrupt = make_interrupt_gate ((unsigned int)irq7, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_SLAVE_BASE + 0].interrupt = make_interrupt_gate ((unsigned int)irq8, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_SLAVE_BASE + 1].interrupt = make_interrupt_gate ((unsigned int)irq9, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_SLAVE_BASE + 2].interrupt = make_interrupt_gate ((unsigned int)irq10, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_SLAVE_BASE + 3].interrupt = make_interrupt_gate ((unsigned int)irq11, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_SLAVE_BASE + 4].interrupt = make_interrupt_gate ((unsigned int)irq12, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_SLAVE_BASE + 5].interrupt = make_interrupt_gate ((unsigned int)irq13, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_SLAVE_BASE + 6].interrupt = make_interrupt_gate ((unsigned int)irq14, KERNEL_CODE_SELECTOR, RING0, PRESENT);
  idt_entry[PIC_SLAVE_BASE + 7].interrupt = make_interrupt_gate ((unsigned int)irq15, KERNEL_CODE_SELECTOR, RING0, PRESENT);

  idt_entry[TRAP_BASE + 0].interrupt = make_interrupt_gate ((unsigned int)trap0, KERNEL_CODE_SELECTOR, RING0, PRESENT);

  idt_flush (&ip);
}

extern "C" void
exception_handler (registers_t regs)
{
  if (interrupt_handler[regs.number]) {
    interrupt_handler[regs.number] (&regs);
  }
  else {
    default_handler (&regs);
  }
}

extern "C" void
irq_handler (registers_t regs)
{
  /* STABLE:  interrupt_handlers[regs.number] != 0.
     This is established in install_idt and maintained by get_interrupt_handler and set_interrupt_handler. */
  interrupt_handler[regs.number] (&regs);

  /* Send end-of-interrupt. */
  if (PIC_SLAVE_BASE <= regs.number && regs.number < PIC_SLAVE_LIMIT) {
    outb (PIC_SLAVE_LOW, PIC_OCW2_LOW | PIC_OCW2_NON_SPECIFIC_EOI);
  }
  outb (PIC_MASTER_LOW, PIC_OCW2_LOW | PIC_OCW2_NON_SPECIFIC_EOI);
}

extern "C" void
trap_handler (registers_t regs)
{
  if (interrupt_handler[regs.number]) {
    interrupt_handler[regs.number] (&regs);
  }
  else {
    default_handler (&regs);
  }
}

void
enable_interrupts ()
{
  asm volatile ("sti");
}

void
disable_interrupts ()
{
  asm volatile ("cli");
}

void
set_interrupt_handler (uint8_t num,
		       interrupt_handler_t handler)
{
  kassert (interrupt_handler[num] == default_handler);
  kassert (handler != 0);

  interrupt_handler[num] = handler;
  
  /* Change the masks on the interrupt controllers. */
  if (PIC_MASTER_BASE <= num && num < PIC_MASTER_LIMIT) {
    pic_master_mask &= ~(1 << (num - PIC_MASTER_BASE));
    outb (PIC_MASTER_HIGH, PIC_OCW1_HIGH | pic_master_mask); 
  }
  else if (PIC_SLAVE_BASE <= num && num < PIC_SLAVE_LIMIT) {
    pic_slave_mask &= ~(1 << (num - PIC_SLAVE_BASE));
    outb (PIC_SLAVE_HIGH, PIC_OCW1_HIGH | pic_slave_mask);
  }

}
