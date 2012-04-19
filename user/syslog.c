#include <automaton.h>
#include <string.h>
#include <buffer_file.h>
#include <dymem.h>
#include <ctype.h>
#include "syslog.h"
#include "kv_parse.h"

/*
  Syslog
  ======
  This automaton receives strings on its input and reproduces, possibly with formatting, on its output.
  The input is auto-parameterized to allow multiple automata to bind.
  The automaton does not produce output until it has been started.
*/

#define INIT_NO 1
#define COM_IN_NO 2
#define COM_OUT_NO 3
#define SYSTEM_EVENT_NO 4
#define TEXT_IN_NO 5
#define TEXT_OUT_NO 6

/* Initialization flag. */
static bool initialized = false;

/* State machine. */
typedef enum {
  ENABLED,
  DISABLED,
} state_t;
static state_t state = DISABLED;

#define COM_IN "[enable! | disable! | status?]"
#define COM_OUT "status = [enabled | disabled]"

static bd_t com_out_bd = -1;
static buffer_file_t com_out_buffer;

static bd_t text_out_bd = -1;
static buffer_file_t text_out_buffer;

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

    text_out_bd = buffer_create (0, 0);
    if (text_out_bd == -1) {
      /* Nothing we can do. */
      exit (-1);
    }
    if (buffer_file_initw (&text_out_buffer, 0, text_out_bd) != 0) {
      /* Nothing we can do. */
      exit (-1);
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
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
      if (strcmp (key, "enable!") == 0) {
	state = ENABLED;
	buffer_file_puts (&com_out_buffer, 0, "status = enabled\n");
      }
      else if (strcmp (key, "disable!") == 0) {
	state = DISABLED;
	buffer_file_puts (&com_out_buffer, 0, "status = disabled\n");
      }
      else if (strcmp (key, "status?") == 0) {
	switch (state) {
	case ENABLED:
	  buffer_file_puts (&com_out_buffer, 0, "status = enabled\n");
	  break;
	case DISABLED:
	  buffer_file_puts (&com_out_buffer, 0, "status = disabled\n");
	  break;
	}
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

/* BEGIN_SYSTEM_INPUT (SYSTEM_EVENT_NO, "system_event", "", system_event, ano_t ano, int param, bd_t bda, bd_t bdb) */
/* { */
/*   initialize (); */
/*   buffer_file_puts (&text_out_buffer, 0, "system_event\n"); */
/*   finish_input (bda, bdb); */
/* } */

BEGIN_SYSTEM_INPUT (SYSTEM_EVENT_NO, "log_event", "", log_event, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();
  
  const log_event_t* le = buffer_map (0, bda);
  bool print_prefix = true;

  for (size_t idx = 0; idx != le->message_size; ++idx) {
    if (print_prefix) {
      print_prefix = false;
      bfprintf (&text_out_buffer, 0, "[%10u.%.3u] %d ", le->time.seconds, le->time.nanoseconds / 1000000, le->aid);
    }

    if (isprint (le->message[idx])) {
      buffer_file_put (&text_out_buffer, 0, le->message[idx]);
    }
    
    print_prefix = (le->message[idx] == '\n');
  }

  if (!print_prefix) {
    buffer_file_put (&text_out_buffer, 0, '\n');
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
  if (buffer_file_initr (&text_in_buffer, 0, bda) != 0) {
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
	if (bfprintf (&text_out_buffer, 0, "(%d) ", aid) < 0) {
	  exit (-1);
	}
      }

      if (buffer_file_put (&text_out_buffer, 0, *begin) != 0) {
	exit (-1);
      }

      print_prefix = (*begin == '\n');

      ++begin;
    }
    
    if (!print_prefix) {
      /* No new line. */
      if (buffer_file_put (&text_out_buffer, 0, '\n') != 0) {
	exit (-1);
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
  return state == ENABLED && buffer_file_size (&text_out_buffer) != 0;
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
    schedule (0, TEXT_OUT_NO, 0);
  }
  if (com_out_precondition ()) {
    schedule (0, COM_OUT_NO, 0);
  }
}
