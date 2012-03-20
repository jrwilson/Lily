#include <automaton.h>
#include <string.h>
#include <buffer_file.h>
#include <dymem.h>
#include "syslog.h"

/*
  Syslog
  ======
  This automaton receives strings on its input and reproduces, possibly with formatting, on its output.
  The input is auto-parameterized to allow multiple automata to bind.
  The automaton does not produce output until it has been started.
*/

#define INIT_NO 1
#define START_NO 2
#define STDIN_NO 3
#define STDOUT_NO 4

/* Initialization flag. */
static bool initialized = false;

/* State machine. */
typedef enum {
  START,
  RUN,
} state_t;
static state_t state = START;

static bd_t stdout_bd = -1;
static buffer_file_t stdout_buffer;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    stdout_bd = buffer_create (0);
    if (stdout_bd == -1) {
      /* Nothing we can do. */
      exit ();
    }
    if (buffer_file_initw (&stdout_buffer, stdout_bd) != 0) {
      /* Nothing we can do. */
      exit ();
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

/* start
   -----
   Change to the run state.
   
   Post: if state == START, then state == RUN.
*/

BEGIN_INPUT (NO_PARAMETER, START_NO, "start", "", start, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (state == START) {
    state = RUN;
  }
    
  finish_input (bda, bdb);
}

/* stdin
   -----
   Receive a line to log.
   
   Post: if successful, then stdout_buffer is not empty
 */
BEGIN_INPUT (AUTO_PARAMETER, STDIN_NO, SYSLOG_STDIN, "buffer_file_t", stdin, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t stdin_buffer;
  if (buffer_file_initr (&stdin_buffer, bda) != 0) {
    finish_input (bda, bdb);
  }

  size_t size = buffer_file_size (&stdin_buffer);
  const char* str = buffer_file_readp (&stdin_buffer, size);
  if (str == 0) {
    finish_input (bda, bdb);
  }

  if (bfprintf (&stdout_buffer, "(%d) ", aid) != 0 ||
      buffer_file_write (&stdout_buffer, str, size) != 0) {
    exit ();
  }

  if (str[size - 1] != '\n') {
    /* No new line. */
    if (buffer_file_put (&stdout_buffer, '\n') != 0) {
      exit ();
    }
  }

  finish_input (bda, bdb);
}

/* stdout
   ------
   Output the contents of the log buffer.
   
   Pre:  state == RUN && stdout_buffer is not empty
   Post: stdout_buffer is empty
 */
static bool
stdout_precondition (void)
{
  return state == RUN && buffer_file_size (&stdout_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, STDOUT_NO, "stdout", "buffer_file_t", stdout, ano_t ano, int param)
{
  initialize ();

  if (stdout_precondition ()) {
    buffer_file_truncate (&stdout_buffer);
    finish_output (true, stdout_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

void
do_schedule (void)
{
  if (stdout_precondition ()) {
    schedule (STDOUT_NO, 0);
  }
}
