#include <automaton.h>
#include <buffer_file.h>
#include "vga_msg.h"
#include "mouse_msg.h"

/*
  Byte Channel
  ============

  Routes inputs and outputs from a terminal server.

  Authors
  -------
  Justin R. Wilson
*/

#define INIT_NO 1
#define TEXT_IN_NO 2
#define TEXT_OUT_NO 3

/* Initialization flag. */
static bool initialized = false;

static bd_t text_out_bd;
static buffer_file_t text_out_buffer;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    text_out_bd = buffer_create (0, 0);
    if (text_out_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&text_out_buffer, 0, text_out_bd) != 0) {
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
  if (buffer_file_initr (&input_buffer, 0, bda) != 0) {
    finish_input (bda, bdb);
  }
  
  size_t size = buffer_file_size (&input_buffer);
  const char* data = buffer_file_readp (&input_buffer, size);
  if (data == 0) {
    finish_input (bda, bdb);
  }
  
  if (buffer_file_write (&text_out_buffer, 0, data, size) != 0) {
    finish_input (bda, bdb);
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

  finish_output (false, -1, -1);
}

void
do_schedule (void)
{
  if (text_out_precondition ()) {
    schedule (0, TEXT_OUT_NO, 0);
  }
}
