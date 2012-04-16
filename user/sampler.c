#include <automaton.h>
#include <buffer_file.h>

#define REQUEST_NO 1
#define RESPONSE_NO 2
#define TEXT_OUT_NO 3
#define START_NO 4

static bool initialized = false;
static bool samp = false;

static bd_t text_out_bd = -1;
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

BEGIN_INPUT (NO_PARAMETER, START_NO, "start", "", start_, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  samp = true;

  finish_input (bda, bdb);
}

BEGIN_OUTPUT (NO_PARAMETER, REQUEST_NO, "request", "", request_, ano_t ano, int param)
{
  initialize ();
  if (samp) {
    samp = false;
    finish_output (true, -1, -1);
  }
  finish_output (false, -1, -1);
}

BEGIN_INPUT (NO_PARAMETER, RESPONSE_NO, "response", "buffer_file_t containing unsigned int", response_, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();
  buffer_file_t buffer;
  if (buffer_file_initr (&buffer, 0, bda) != 0) {
    finish_input (bda, bdb);
  }

  unsigned int tick;
  if (buffer_file_read (&buffer, &tick, sizeof (unsigned int)) != 0) {
    finish_input (bda, bdb);
  }

  bfprintf (&text_out_buffer, 0, "tick = %d\n", tick);

  finish_input (bda, bdb);
}

BEGIN_OUTPUT (NO_PARAMETER, TEXT_OUT_NO, "text_out", "buffer_file_t", text_out, ano_t ano, int param)
{
  initialize ();
  if (buffer_file_size (&text_out_buffer) != 0) {
    buffer_file_truncate (&text_out_buffer);
    finish_output (true, text_out_bd, -1);
  }
  finish_output (false, -1, -1);
}

void
do_schedule (void)
{
  if (samp) {
    schedule (0, REQUEST_NO, 0);
  }
  if (buffer_file_size (&text_out_buffer) != 0) {
    schedule (0, TEXT_OUT_NO, 0);
  }
}
