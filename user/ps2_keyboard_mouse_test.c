#include <automaton.h>
#include <fifo_scheduler.h>
#include <string.h>
#include <buffer_file.h>
#include "mouse_msg.h"

/*
  Driver for a PS2 mouse
  ================================
  This driver converts bytes encoding mouse movement and button status values into a consistent mouse message format.
  Open questions include how z movement works (discrete for vertical vs. horizontal?) and how to use time stamps.

  Design
  ======
  
  Based on the wiki.osdev.net PS2 mouse documentation - we assume what is said there is truth.
*/

#define INIT_NO 1
#define SCAN_CODES_IN_NO 2
#define MOUSE_PACKETS_IN_NO 3
#define STDOUT_NO 4

/* Initialization flag. */
static bool initialized = false;

static bd_t stdout_bd = -1;
static buffer_file_t stdout_buffer;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    /* Allocate the output buffer. */
    stdout_bd = buffer_create (0);
    if (stdout_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&stdout_buffer, stdout_bd) != 0) {
      exit ();
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  end_internal_action ();
}

BEGIN_INPUT (NO_PARAMETER, SCAN_CODES_IN_NO, "scan_codes_in", "buffer_file_t", scan_codes_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) != 0) {
    end_input_action (bda, bdb);
  }

  size_t size = buffer_file_size (&input_buffer);
  const unsigned char* codes = buffer_file_readp (&input_buffer, size);
  if (codes == 0) {
    end_input_action (bda, bdb);
  }

  for (size_t idx = 0; idx != size; ++idx) {
    bfprintf (&stdout_buffer, "scan code %x\n", codes[idx]);
  }

  end_input_action (bda, bdb);
}

BEGIN_INPUT (NO_PARAMETER, MOUSE_PACKETS_IN_NO, "mouse_packets_in", "mouse_packet_list_t", mouse_packets_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  size_t count = 0;
  mouse_packet_t mp;
  mouse_packet_list_t input_buffer;
  if (mouse_packet_list_initr (&input_buffer, bda, &count) != 0) {
    end_input_action (bda, bdb);
  }

  for (size_t idx = 0; idx != count; ++idx) {
    if (mouse_packet_list_read (&input_buffer, &mp) != 0) {
      end_input_action (bda, bdb);
    }
    
    bfprintf (&stdout_buffer,
	      "status %x dx %d dy %d dzv %d dzh %d %d %d\n",
	      mp.button_status_bits,
	      mp.x_delta,
	      mp.y_delta,
	      mp.z_delta_vertical,
	      mp.z_delta_horizontal,
	      mp.time_stamp.seconds,
	      mp.time_stamp.nanoseconds);
  }

  end_input_action (bda, bdb);
}

static bool
stdout_precondition (void)
{
  return buffer_file_size (&stdout_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, STDOUT_NO, "stdout", "buffer_file_t", stdout, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (ano, param);

  if (stdout_precondition ()) {
    buffer_file_truncate (&stdout_buffer);
    end_output_action (true, stdout_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

void
schedule (void)
{
  if (stdout_precondition ()) {
    scheduler_add (STDOUT_NO, 0);
  }
}
