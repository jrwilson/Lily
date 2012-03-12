#include <automaton.h>
#include <fifo_scheduler.h>
#include <string.h>
#include <buffer_file.h>
#include "ps2_mouse_msg.h"

/*
  Driver for a PS2 mouse
  ================================
  This driver converts bytes encoding mouse movement and button status values into a consistent mouse message format.
  Open questions include how z movement works (discrete for vertical vs. horizontal?) and how to use time stamps.

  Design
  ======
  
  Based on the wiki.osdev.net PS2 mouse documentation - we assume what is said there is truth.
*/

#define MOUSE_PACKET_IN_NO 1
#define MOUSE_PACKET_OUT_NO 2

/* Initialization flag. */
static bool initialized = false;

/* Processed scan codes. */
static bd_t output_buffer_bd = -1;
static buffer_file_t output_buffer;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    /* Allocate the output buffer. */
    output_buffer_bd = buffer_create (0);
    if (output_buffer_bd == -1) {
      syslog ("ps2_mouse_test: error: Could not create output buffer");
      exit ();
    }
    if (buffer_file_initw (&output_buffer, output_buffer_bd) == -1) {
      syslog ("ps2_mouse_test: error: Could not initialize output buffer");
      exit ();
    }
  }
}

static void
schedule (void);

static void
end_input_action (bd_t bda,
		  bd_t bdb)
{
  if (bda != -1) {
    buffer_destroy (bda);
  }
  if (bdb != -1) {
    buffer_destroy (bdb);
  }
  schedule ();
  scheduler_finish (false, -1, -1);
}

static void
end_output_action (bool output_fired,
		   bd_t bda,
		   bd_t bdb)
{
  schedule ();
  scheduler_finish (output_fired, bda, bdb);
}

/* mouse packet
   ---------
   Convert scancodes into ASCII text.
   
   Post: if successful, then output_buffer_initialized == true && the output buffer is not empty
 */
BEGIN_INPUT (NO_PARAMETER, MOUSE_PACKET_IN_NO, "mouse_packet_in", "ps2_mouse_packet_list_t", mouse_packet_in, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  size_t count = 0;
  mouse_packet_t mp;
  ps2_mouse_packet_list_t input_buffer;
  if (ps2_mouse_packet_list_initr (&input_buffer, bda, &count) == -1) {
    end_input_action (bda, bdb);
  }

  for (size_t idx = 0; idx != count; ++idx) {

    if (ps2_mouse_packet_list_read (&input_buffer,
				    &mp) != 0) {
      end_input_action (bda, bdb);
    }
    /* TODO print out time stamp once its implemented */
    bfprintf (&output_buffer,
	      "status %x dx %d dy %d dzv %d dzh %d \n",
	      mp.button_status_bits,
	      mp.x_delta,
	      mp.y_delta,
	      mp.z_delta_vertical,
	      mp.z_delta_horizontal);
  }

  end_input_action (bda, bdb);
}

/* text
   ----
   Output a string of ASCII text.
   
   Pre:  output_buffer_initialized == true && the output buffer is not empty
   Post: output_buffer_initialized == false
 */
static bool
mouse_packet_out_precondition (void)
{
  return buffer_file_size (&output_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, MOUSE_PACKET_OUT_NO, "mouse_packet_out", "buffer_file", mouse_packet_out, int param)
{
  initialize ();
  scheduler_remove (MOUSE_PACKET_OUT_NO, param);

  if (mouse_packet_out_precondition ()) {
    buffer_file_truncate (&output_buffer);
    end_output_action (true, output_buffer_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

static void
schedule (void)
{
  if (mouse_packet_out_precondition ()) {
    scheduler_add (MOUSE_PACKET_OUT_NO, 0);
  }
}
