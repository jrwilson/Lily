#include <automaton.h>
#include <buffer_queue.h>
#include <io.h>
#include <fifo_scheduler.h>
#include <string.h>
#include <buffer_file.h>

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

/* Initialization flag. */
static bool initialized = false;

/* Queue of buffers that need to be destroyed. */
static buffer_queue_t destroy_queue;

/* Output buffer containing scan codes. */
static buffer_file_t output_buffer;
static bd_t output_buffer_bd = -1;

#define KEYBOARD_INTERRUPT_NO 1
#define SCAN_CODE_NO 2
#define DESTROY_BUFFERS_NO 3

static void
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb);

static void
initialize_output_buffer (void)
{
  output_buffer_bd = buffer_create (1);
  if (output_buffer_bd == -1) {
    syslog ("keyboard: error:  Could not create output buffer");
    exit ();
  }      
  if (buffer_file_initc (&output_buffer, output_buffer_bd) == -1) {
    syslog ("keyboard: error:  Could not initialize output buffer");
    exit ();
  }
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    buffer_queue_init (&destroy_queue);
    initialize_output_buffer ();

    /* Reserve system resources. */
    if (reserve_port (KEYBOARD_DATA_PORT) == -1) {
      syslog ("keyboard: error:  Could not reserve data port");
      exit ();
    }
    if (reserve_port (KEYBOARD_CONTROLLER_PORT) == -1) {
      syslog ("keyboard: error:  Could not reserve controller port");
      exit ();
    }
    if (subscribe_irq (KEYBOARD_IRQ, KEYBOARD_INTERRUPT_NO, 0) == -1) {
      syslog ("keyboard: error:  Could not subscribe to keyboard irq");
      exit ();
    }
  }
}

BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  end_action (false, bda, bdb);
}

BEGIN_SYSTEM_INPUT (KEYBOARD_INTERRUPT_NO, "", "", keyboard_interrupt, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  char c = inb (KEYBOARD_DATA_PORT);
  if (buffer_file_write (&output_buffer, &c, 1) == -1) {
    syslog ("keyboard: error:  Could not write to output buffer");
    exit ();
  }
  end_action (false, bda, bdb);
}

static bool
scan_code_precondition (void)
{
  return buffer_file_size (&output_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, SCAN_CODE_NO, "scan_code", "buffer_file", scan_code, int param)
{
  initialize ();
  scheduler_remove (SCAN_CODE_NO, param);

  if (scan_code_precondition ()) {
    bd_t bd = output_buffer_bd;
    initialize_output_buffer ();
    end_action (true, bd, -1);
  }
  else {
    end_action (false, -1, -1);
  }
}

/* destroy_buffers
   ---------------
   Destroys all of the buffers in destroy_queue.
   This is useful for output actions that need to destroy the buffer *after* the output has fired.
   To schedule a buffer for destruction, just add it to destroy_queue.

   Pre:  Destroy queue is not empty.
   Post: Destroy queue is empty.
 */
static bool
destroy_buffers_precondition (void)
{
  return !buffer_queue_empty (&destroy_queue);
}

BEGIN_INTERNAL (NO_PARAMETER, DESTROY_BUFFERS_NO, "", "", destroy_buffers, int param)
{
  initialize ();
  scheduler_remove (DESTROY_BUFFERS_NO, param);

  if (destroy_buffers_precondition ()) {
    while (!buffer_queue_empty (&destroy_queue)) {
      bd_t bd;
      const buffer_queue_item_t* item = buffer_queue_front (&destroy_queue);
      bd = buffer_queue_item_bda (item);
      if (bd != -1) {
	buffer_destroy (bd);
      }
      bd = buffer_queue_item_bdb (item);
      if (bd != -1) {
	buffer_destroy (bd);
      }

      buffer_queue_pop (&destroy_queue);
    }
  }

  end_action (false, -1, -1);
}

/* end_action is a helper function for terminating actions.
   If the buffer is not -1, it schedules it to be destroyed.
   end_action schedules local actions and calls scheduler_finish to finish the action.
*/
static void
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb)
{
  if (bda != -1 || bdb != -1) {
    buffer_queue_push (&destroy_queue, 0, bda, 0, bdb, 0);
  }

  if (scan_code_precondition ()) {
    scheduler_add (SCAN_CODE_NO, 0);
  }
  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }

  scheduler_finish (output_fired, bda, bdb);
}
