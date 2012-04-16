#include <automaton.h>
#include <buffer_file.h>

#define INIT_NO 1
#define TICK_NO 2
#define REQUEST_NO 3
#define RESPONSE_NO 4

static bool initialized = false;
static bd_t response_bd = -1;
static buffer_file_t response_buffer;

static unsigned int tick = 0;
static bool req = false;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;
    response_bd = buffer_create (0, 0);
    if (response_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&response_buffer, 0, response_bd) != 0) {
      exit ();
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init_, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

BEGIN_INTERNAL (NO_PARAMETER, TICK_NO, "tick", "", tick_, ano_t ano, int param)
{
  initialize ();
  ++tick;
  finish_internal ();
}

BEGIN_INPUT (NO_PARAMETER, REQUEST_NO, "request", "", request_, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();
  req = true;
  finish_input (bda, bdb);
}

BEGIN_OUTPUT (NO_PARAMETER, RESPONSE_NO, "response", "buffer_file_t containing unsigned int", response_, ano_t ano, int param)
{
  initialize ();
  if (req) {
    req = false;
    buffer_file_truncate (&response_buffer);
    if (buffer_file_write (&response_buffer, 0, &tick, sizeof (unsigned int)) != 0) {
      exit ();
    }
    finish_output (true, response_bd, -1);
  }
  finish_output (false, -1, -1);
}

void
do_schedule (void)
{
  schedule (0, TICK_NO, 0);
  if (req) {
    schedule (0, RESPONSE_NO, 0);
  }
}
