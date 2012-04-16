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
#define TEXT_IN_NO 3
#define TEXT_OUT_NO 4

/* Initialization flag. */
static bool initialized = false;

/* State machine. */
typedef enum {
  START,
  RUN,
} state_t;
static state_t state = START;

static bd_t text_out_bd = -1;
static buffer_file_t text_out_buffer;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    text_out_bd = buffer_create (0, 0);
    if (text_out_bd == -1) {
      /* Nothing we can do. */
      exit ();
    }
    if (buffer_file_initw (&text_out_buffer, text_out_bd) != 0) {
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

/* text_in
   -------
   Receive a line to log.
   
   Post: if successful, then text_out_buffer is not empty
 */
BEGIN_INPUT (AUTO_PARAMETER, TEXT_IN_NO, SYSLOG_TEXT_IN, "buffer_file_t", text_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t text_in_buffer;
  if (buffer_file_initr (&text_in_buffer, bda) != 0) {
    finish_input (bda, bdb);
  }

  size_t size = buffer_file_size (&text_in_buffer);
  const char* str = buffer_file_readp (&text_in_buffer, size);
  if (str == 0) {
    finish_input (bda, bdb);
  }

  if (size != 0) {
    const char* begin = str;
    const char* end = begin + size;
    bool print_prefix = true;
    
    while (begin != end) {
      if (print_prefix) {
	print_prefix = false;
	/* Print the prefix. */
	if (bfprintf (&text_out_buffer, "(%d) ", aid) != 0) {
	  exit ();
	}
      }

      if (buffer_file_put (&text_out_buffer, *begin) != 0) {
	exit ();
      }

      print_prefix = (*begin == '\n');

      ++begin;
    }
    
    if (!print_prefix) {
      /* No new line. */
      if (buffer_file_put (&text_out_buffer, '\n') != 0) {
	exit ();
      }
    }
  }

  finish_input (bda, bdb);
}

/* text_out
   ------
   Output the contents of the log buffer.
   
   Pre:  state == RUN && text_out_buffer is not empty
   Post: text_out_buffer is empty
 */
static bool
text_out_precondition (void)
{
  return state == RUN && buffer_file_size (&text_out_buffer) != 0;
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
    schedule (TEXT_OUT_NO, 0, 0);
  }
}
