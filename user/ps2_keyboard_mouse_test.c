#include <automaton.h>
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
#define TEXT_IN_NO 3
#define MOUSE_PACKETS_IN_NO 4
#define TEXT_OUT_NO 5

/* Initialization flag. */
static bool initialized = false;

static bd_t text_out_bd = -1;
static buffer_file_t text_out_buffer;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    /* Allocate the output buffer. */
    text_out_bd = buffer_create (0);
    if (text_out_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&text_out_buffer, text_out_bd) != 0) {
      exit ();
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

BEGIN_INPUT (NO_PARAMETER, TEXT_IN_NO, "text_in", "buffer_file_t", text_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) != 0) {
    finish_input (bda, bdb);
  }

  size_t size = buffer_file_size (&input_buffer);
  const char* codes = buffer_file_readp (&input_buffer, size);
  if (codes == 0) {
    finish_input (bda, bdb);
  }

  for (size_t idx = 0; idx != size; ++idx) {
    bfprintf (&text_out_buffer, "ascii %c (%d)\n", codes[idx], codes[idx]);
  }

  finish_input (bda, bdb);
}

BEGIN_INPUT (NO_PARAMETER, MOUSE_PACKETS_IN_NO, "mouse_packets_in", "mouse_packet_list_t", mouse_packets_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  size_t count = 0;
  mouse_packet_t mp;
  mouse_packet_list_t input_buffer;
  if (mouse_packet_list_initr (&input_buffer, bda, &count) != 0) {
    finish_input (bda, bdb);
  }

  for (size_t idx = 0; idx != count; ++idx) {
    if (mouse_packet_list_read (&input_buffer, &mp) != 0) {
      finish_input (bda, bdb);
    }
    
    bfprintf (&text_out_buffer,
	      "status %x dx %d dy %d dzv %d dzh %d %d %d\n",
	      mp.button_status_bits,
	      mp.x_delta,
	      mp.y_delta,
	      mp.z_delta_vertical,
	      mp.z_delta_horizontal,
	      mp.time_stamp.seconds,
	      mp.time_stamp.nanoseconds);
  }

  finish_input (bda, bdb);
}

static bool
text_out_precondition (void)
{
  return buffer_file_size (&text_out_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, TEXT_OUT_NO, "text_out", "buffer_file_t", text_out, ano_t ano, int param)
{
  initialize ();

  if (text_out_precondition ()) {
    buffer_file_truncate (&text_out_buffer);
    finish_output (true, text_out_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

void
do_schedule (void)
{
  if (text_out_precondition ()) {
    schedule (TEXT_OUT_NO, 0);
  }
}
