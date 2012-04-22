#include <automaton.h>
#include <buffer_file.h>
#include <string.h>
#include <ctype.h>

/*
  Syslog
  ======
  This automaton receives logging and system events from the kernel and formats them for output.
  The automaton does not produce output until its text_out output is bound.
*/

#define LOG_EVENT_NO 1
#define SYSTEM_EVENT_NO 2
#define TEXT_OUT_NO 3

/* Initialization flag. */
static bool initialized = false;

static bool text_out_bound = true;
static bd_t text_out_bd = -1;
static buffer_file_t text_out_buffer;

#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    text_out_bd = buffer_create (0);
    if (text_out_bd == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create text_out buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (buffer_file_initw (&text_out_buffer, text_out_bd) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize text_out buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
  }
}

BEGIN_SYSTEM_INPUT (LOG_EVENT_NO, "log_event", "", log_event, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();
  
  const log_event_t* le = buffer_map (bda);
  if (le == 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not map log event buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }

  bool print_prefix = true;

  for (size_t idx = 0; idx != le->message_size; ++idx) {
    if (print_prefix) {
      print_prefix = false;
      if (bfprintf (&text_out_buffer, "[%10u.%.3u] %d ", le->time.seconds, le->time.nanoseconds / 1000000, le->aid) < 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to text_out buffer: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }
    }

    if (isprint (le->message[idx])) {
      if (buffer_file_put (&text_out_buffer, le->message[idx]) != 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to text_out buffer: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }
    }
    
    print_prefix = (le->message[idx] == '\n');
  }

  if (!print_prefix) {
    if (buffer_file_put (&text_out_buffer, '\n') != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to text_out buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
  }

  finish_input (bda, bdb);
}

BEGIN_SYSTEM_INPUT (SYSTEM_EVENT_NO, "system_event", "", system_event, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();
  if (buffer_file_puts (&text_out_buffer, "system_event\n") != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to text_out buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    exit (-1);
  }
  finish_input (bda, bdb);
}

static bool
text_out_precondition (void)
{
  return text_out_bound && buffer_file_size (&text_out_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, TEXT_OUT_NO, "text_out", "buffer_file_t", text_out, ano_t ano, int param, size_t bc)
{
  initialize ();

  text_out_bound = (bc != 0);

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
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not schedule text_out action: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
  }
}
