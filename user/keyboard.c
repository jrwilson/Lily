#include "keyboard.h"
#include <automaton.h>
#include <io.h>
#include <buffer_heap.h>
#include <fifo_scheduler.h>
#include "string_buffer.h"

/*
  Keyboard Driver
  ===============

  The keyboard system is organized according to the following diagram:

    CPU <=> Keyboard Controller <-> Keyboard

  The keyboard controller communicates with the keyboard via a serial link.
  The keyboard controller communicates with the CPU using I/O ports and an interrupt.
  This driver is for AT+ architectures, i.e., the keyboard contains an 8031 controller and the keyboard controller consists of a 8042 controller.

  The main interface consists of the SCANCODE output action that delivers scan codes from the keyboard.
  If there is demand, the Kscan codes could be delivered instead of the scan codes.
 */

/* Data read/written on this port passes through the keyboard controller to the keyboard. */
#define KEYBOARD_DATA_PORT 0x60
/* Data read/written on this port pertains to the keyboard controller. */
#define KEYBOARD_CONTROLLER_PORT 0x64
/* This is the interrrupt request generated by the keyboard controller. */
#define KEYBOARD_IRQ 1

/* Describes the status byte of the keyboard controller. */
typedef union {
  unsigned char data;
  struct {
    unsigned char output_buffer_ready : 1;
    unsigned char input_buffer_full : 1;
    unsigned char self_test : 1;
    unsigned char last_xfer_command : 1; /* Otherwise, data. */
    unsigned char keyboard_enabled : 1;
    unsigned char transmit_timeout_error : 1;
    unsigned char receive_timeout_error : 1;
    unsigned char parity_error : 1;
  } bits;
} status_t;

#define KEYBOARD_INTERRUPT 1

/* Initial size of the scan code array. */
#define INITIAL_CAPACITY 4000

/*
  An array of scan codes implemented with a buffer.
  This allows us to output the array when the SCAN_CODE output fires.
*/
static string_buffer_t string_buffer;

static void schedule (void);

void
init (int param,
      bd_t bd,
      void* ptr,
      size_t buffer_size)
{
  /* Reserve system resources. */
  reserve_port (KEYBOARD_DATA_PORT);
  reserve_port (KEYBOARD_CONTROLLER_PORT);
  subscribe_irq (KEYBOARD_IRQ, KEYBOARD_INTERRUPT, 0);

  string_buffer_harvest (&string_buffer, INITIAL_CAPACITY);

  schedule ();
  scheduler_finish (-1, FINISH_NO);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);

void
interrupt (int param)
{
  string_buffer_put (&string_buffer, inb (KEYBOARD_DATA_PORT));

  schedule ();
  scheduler_finish (-1, FINISH_NO);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, KEYBOARD_INTERRUPT, interrupt);

static bool
keycode_precondition (void)
{
  /* We don't care if we are bound or not. */
  return string_buffer_size (&string_buffer) != 0;
}

void
scan_code (int param,
	   size_t binding_count)
{
  scheduler_remove (KEYBOARD_SCAN_CODE, param);

  if (keycode_precondition ()) {
    bd_t bd = string_buffer_harvest (&string_buffer, INITIAL_CAPACITY);
    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, KEYBOARD_SCAN_CODE, scan_code);

static void
schedule (void)
{
  if (keycode_precondition ()) {
    scheduler_add (KEYBOARD_SCAN_CODE, 0);
  }
}
