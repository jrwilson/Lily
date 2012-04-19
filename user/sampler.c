#include <automaton.h>
#include <buffer_file.h>
#include <string.h>
#include <dymem.h>
#include "kv_parse.h"

#define REQUEST_NO 1
#define RESPONSE_NO 2
#define COM_IN_NO 4
#define COM_OUT_NO 5

static bool initialized = false;
static bool samp = false;

#define COM_IN "[sample!]"
#define COM_OUT "ticks = TICKS"

static bd_t com_out_bd = -1;
static buffer_file_t com_out_buffer;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    com_out_bd = buffer_create (0, 0);
    if (com_out_bd == -1) {
      /* Nothing we can do. */
      exit (-1);
    }
    if (buffer_file_initw (&com_out_buffer, 0, com_out_bd) != 0) {
      /* Nothing we can do. */
      exit (-1);
    }
  }
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

  bfprintf (&com_out_buffer, 0, "ticks = %d\n", tick);

  finish_input (bda, bdb);
}

BEGIN_INPUT (NO_PARAMETER, COM_IN_NO, "com_in", COM_IN, com_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t bf;
  if (buffer_file_initr (&bf, 0, bda) != 0) {
    finish_input (bda, bdb);
  }

  const size_t size = buffer_file_size (&bf);
  const char* begin = buffer_file_readp (&bf, size);
  if (begin == 0) {
    finish_input (bda, bdb);
  }
  const char* end = begin + size;
  const char* ptr = begin;
  
  char* key = 0;
  char* value = 0;
  while (ptr != end && kv_parse (0, &key, &value, &ptr, end) == 0) {
    if (key != 0) {
      if (strcmp (key, "sample!") == 0) {
	samp = true;
      }
      else {
	bfprintf (&com_out_buffer, 0, "unknown label `%s'\n", key);
      }
    }
    free (key);
    free (value);
  }

  finish_input (bda, bdb);
}

static bool
com_out_precondition (void)
{
  return buffer_file_size (&com_out_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, COM_OUT_NO, "com_out", COM_OUT, com_out, ano_t ano, int param)
{
  initialize ();

  if (com_out_precondition ()) {
    buffer_file_truncate (&com_out_buffer);
    finish_output (true, com_out_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

void
do_schedule (void)
{
  if (samp) {
    schedule (0, REQUEST_NO, 0);
  }
  if (buffer_file_size (&com_out_buffer) != 0) {
    schedule (0, COM_OUT_NO, 0);
  }
}
