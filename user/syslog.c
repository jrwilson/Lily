#include <automaton.h>
#include <buffer_file.h>
#include <string.h>
#include <ctype.h>
#include "system.h"

/*
  Syslog
  ======
  This automaton receives logging and system events from the kernel and formats them for output.

  TODO:  The automaton should not produce output until its text_out output is bound.
*/

#define INIT_NO 1
#define LOG_EVENT_NO 2
#define TEXT_OUT_NO 3

/* Initialization flag. */
static bool initialized = false;

static bd_t text_out_bd = -1;
static buffer_file_t text_out_buffer;

#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "

BEGIN_INPUT (NO_PARAMETER, INIT_NO, SA_INIT_IN_NAME, "", init, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  if (!initialized) {
    initialized = true;

    text_out_bd = buffer_create (0);
    if (text_out_bd == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create text_out buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (buffer_file_initw (&text_out_buffer, text_out_bd) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize text_out buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
  }

  finish_input (bda, bdb);
}

/* BEGIN_INTERNAL (LOG_EVENT_NO, "log_event", "", log_event, ano_t ano, int param, bd_t bda, bd_t bdb) */
/* { */
/*   initialize (); */
  
/*   const log_event_t* le = buffer_map (bda); */
/*   if (le == 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not map log event buffer: %s\n", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   if (bfprintf (&text_out_buffer, "[%10u.%.3u] %d ", le->time.seconds, le->time.nanoseconds / 1000000, le->aid) < 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to text_out buffer: %s\n", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   if (buffer_file_write (&text_out_buffer, le->message, le->message_size) != 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to text_out buffer: %s\n", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */
  
/*   if (buffer_file_put (&text_out_buffer, '\n') != 0) { */
/*     snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to text_out buffer: %s\n", lily_error_string (lily_error)); */
/*     logs (log_buffer); */
/*     exit (-1); */
/*   } */

/*   finish_input (bda, bdb); */
/* } */

static bool
text_out_precondition (void)
{
  return buffer_file_size (&text_out_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, TEXT_OUT_NO, "text_out", "buffer_file_t", text_out, ano_t ano, int param)
{
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
    if (schedule (TEXT_OUT_NO, 0) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not schedule text_out action: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
  }
}
