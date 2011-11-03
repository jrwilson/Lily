/*
  File
  ----
  pit.c
  
  Description
  -----------
  Functions for controlling the programmable interrupt timer (pit).

  Authors
  -------
  http://www.jamesmolloy.co.uk/tutorial_html/5.-IRQs%20and%20the%20PIT.html
  http://en.wikipedia.org/wiki/Intel_8253
  Justin R. Wilson
*/

#include "pit.h"
#include "io.h"
#include "idt.h"
#include "kput.h"

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

static unsigned int tick = 0;

static void
pit_handler (/*registers_t* regs*/)
{
  kputs ("Tick="); kputuix (tick); kputs ("\n");
  ++tick;
}

void
initialize_pit (unsigned short period)
{
  set_interrupt_handler (32, pit_handler);

  /* Send the command byte. */
  outb (PIT_COMMAND, PIT_WRITE_CHANNEL0 | PIT_LOW_HIGH | PIT_MODE3 | PIT_BINARY);
  
  /* Send the period. */
  outb (PIT_CHANNEL0, period & 0xFF);
  outb (PIT_CHANNEL0, (period >> 8) & 0xFF);
}
