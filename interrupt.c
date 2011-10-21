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

#define INTERRUPT_COUNT 256

static idt_entry_t idt[INTERRUPT_COUNT];
static idt_ptr_t ip;

extern void tsr0 ();
extern void tsr1 ();
extern void tsr2 ();
extern void tsr3 ();
extern void tsr4 ();
extern void tsr5 ();
extern void tsr6 ();
extern void tsr7 ();
extern void tsr8 ();
extern void tsr9 ();
extern void tsr10 ();
extern void tsr11 ();
extern void tsr12 ();
extern void tsr13 ();
extern void tsr14 ();
extern void tsr15 ();
extern void tsr16 ();
extern void tsr17 ();
extern void tsr18 ();
extern void tsr19 ();
extern void tsr20 ();
extern void tsr21 ();
extern void tsr22 ();
extern void tsr23 ();
extern void tsr24 ();
extern void tsr25 ();
extern void tsr26 ();
extern void tsr27 ();
extern void tsr28 ();
extern void tsr29 ();
extern void tsr30 ();
extern void tsr31 ();

extern void isr0 ();
extern void isr1 ();
extern void isr2 ();
extern void isr3 ();
extern void isr4 ();
extern void isr5 ();
extern void isr6 ();
extern void isr7 ();
extern void isr8 ();
extern void isr9 ();
extern void isr10 ();
extern void isr11 ();
extern void isr12 ();
extern void isr13 ();
extern void isr14 ();
extern void isr15 ();

extern void
idt_flush (idt_ptr_t*);

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

void
install_idt ()
{
  ip.limit = (sizeof (idt_entry_t) * INTERRUPT_COUNT) - 1;
  ip.base = idt;

  int k;
  for (k = 0; k < INTERRUPT_COUNT; ++k) {
    idt_set_gate (k, 0, 0, 0);
  }

  idt_set_gate (0, (unsigned int)tsr0, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (1, (unsigned int)tsr1, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (2, (unsigned int)tsr2, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (3, (unsigned int)tsr3, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (4, (unsigned int)tsr4, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (5, (unsigned int)tsr5, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (6, (unsigned int)tsr6, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (7, (unsigned int)tsr7, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (8, (unsigned int)tsr8, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (9, (unsigned int)tsr9, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (10, (unsigned int)tsr10, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (11, (unsigned int)tsr11, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (12, (unsigned int)tsr12, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (13, (unsigned int)tsr13, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (14, (unsigned int)tsr14, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (15, (unsigned int)tsr15, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (16, (unsigned int)tsr16, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (17, (unsigned int)tsr17, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (18, (unsigned int)tsr18, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (19, (unsigned int)tsr19, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (20, (unsigned int)tsr20, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (21, (unsigned int)tsr21, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (22, (unsigned int)tsr22, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (23, (unsigned int)tsr23, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (24, (unsigned int)tsr24, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (25, (unsigned int)tsr25, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (26, (unsigned int)tsr26, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (27, (unsigned int)tsr27, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (28, (unsigned int)tsr28, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (29, (unsigned int)tsr29, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (30, (unsigned int)tsr30, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);
  idt_set_gate (31, (unsigned int)tsr31, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | TRAP_GATE_32);

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

  idt_set_gate (PIC_MASTER_BASE + 0, (unsigned int)isr0, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 1, (unsigned int)isr1, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 2, (unsigned int)isr2, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 3, (unsigned int)isr3, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 4, (unsigned int)isr4, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 5, (unsigned int)isr5, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 6, (unsigned int)isr6, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_MASTER_BASE + 7, (unsigned int)isr7, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 0, (unsigned int)isr8, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 1, (unsigned int)isr9, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 2, (unsigned int)isr10, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 3, (unsigned int)isr11, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 4, (unsigned int)isr12, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 5, (unsigned int)isr13, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 6, (unsigned int)isr14, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);
  idt_set_gate (PIC_SLAVE_BASE + 7, (unsigned int)isr15, KERNEL_CODE_SEGMENT, PRESENT | RING0 | SYSTEM | INTERRUPT_GATE_32);

  idt_flush (&ip);
}

struct registers
{
  unsigned int ds;
  unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
  unsigned int number;
  unsigned int error;
  unsigned int eip, cs, eflags, useresp, ss;
};
typedef struct registers registers_t;

void
tsr_handler (registers_t regs)
{
  kputs ("Trap: "); kputux (regs.number); kputs (" Code: " ); kputux (regs.error); kputs ("\n");

  kputs ("CS: "); kputux (regs.cs); kputs (" EIP: "); kputux (regs.eip); kputs (" EFLAGS: "); kputux (regs.eflags); kputs ("\n");
  kputs ("SS: "); kputux (regs.ss); kputs (" ESP: "); kputux (regs.useresp); kputs (" DS:"); kputux (regs.ds); kputs ("\n");

  kputs ("EAX: "); kputux (regs.eax); kputs (" EBX: "); kputux (regs.ebx); kputs (" ECX: "); kputux (regs.ecx); kputs (" EDX: "); kputux (regs.edx); kputs ("\n");
  kputs ("ESP: "); kputux (regs.esp); kputs (" EBP: "); kputux (regs.ebp); kputs (" ESI: "); kputux (regs.esi); kputs (" EDI: "); kputux (regs.edi); kputs ("\n");
}

void
isr_handler (registers_t regs)
{
  kputs ("Trap: "); kputux (regs.number); kputs (" Code: " ); kputux (regs.error); kputs ("\n");

  kputs ("CS: "); kputux (regs.cs); kputs (" EIP: "); kputux (regs.eip); kputs (" EFLAGS: "); kputux (regs.eflags); kputs ("\n");
  kputs ("SS: "); kputux (regs.ss); kputs (" ESP: "); kputux (regs.useresp); kputs (" DS:"); kputux (regs.ds); kputs ("\n");

  kputs ("EAX: "); kputux (regs.eax); kputs (" EBX: "); kputux (regs.ebx); kputs (" ECX: "); kputux (regs.ecx); kputs (" EDX: "); kputux (regs.edx); kputs ("\n");
  kputs ("ESP: "); kputux (regs.esp); kputs (" EBP: "); kputux (regs.ebp); kputs (" ESI: "); kputux (regs.esi); kputs (" EDI: "); kputux (regs.edi); kputs ("\n");

  /* Send end-of-interrupt. */
  if (PIC_SLAVE_BASE <= regs.number && regs.number < PIC_SLAVE_LIMIT) {
    outb (PIC_SLAVE_LOW, PIC_OCW2_LOW | PIC_OCW2_NON_SPECIFIC_EOI);
  }
  outb (PIC_MASTER_LOW, PIC_OCW2_LOW | PIC_OCW2_NON_SPECIFIC_EOI);
}
