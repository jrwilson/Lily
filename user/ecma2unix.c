#include <automaton.h>
#include <buffer_file.h>
#include "vga_msg.h"
#include "mouse_msg.h"

/*
  ECMA48 to UNIX
  ==============

  Convert I/O from an ECMA48 terminal to the UNIX convention.

  Authors
  -------
  Justin R. Wilson
*/

#define INIT_NO 1
#define TEXT_IN_TERM_NO 2
#define MOUSE_PACKETS_IN_TERM_NO 3
#define TEXT_OUT_NO 4
#define MOUSE_PACKETS_OUT_NO 5
#define TEXT_IN_NO 6
#define TEXT_OUT_TERM_NO 7

/* Initialization flag. */
static bool initialized = false;

static bd_t text_out_bd;
static buffer_file_t text_out_buffer;

static bd_t mouse_packets_bd;
static mouse_packet_list_t mouse_packet_list;

static bd_t text_out_term_bd;
static buffer_file_t text_out_term_buffer;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    text_out_bd = buffer_create (0);
    if (text_out_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&text_out_buffer, text_out_bd) != 0) {
      exit ();
    }

    mouse_packets_bd = buffer_create (0);
    if (mouse_packets_bd == -1) {
      exit ();
    }
    if (mouse_packet_list_initw (&mouse_packet_list, mouse_packets_bd) != 0) {
      exit ();
    }

    text_out_term_bd = buffer_create (0);
    if (text_out_term_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&text_out_term_buffer, text_out_term_bd) != 0) {
      exit ();
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

BEGIN_INPUT (NO_PARAMETER, TEXT_IN_TERM_NO, "text_in_term", "buffer_file_t", text_in_term, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) != 0) {
    finish_input (bda, bdb);
  }
  
  size_t size = buffer_file_size (&input_buffer);
  const char* data = buffer_file_readp (&input_buffer, size);
  if (data == 0) {
    finish_input (bda, bdb);
  }
  
  const char* begin = data;
  const char* end = data + size;

  for (; begin != end; ++begin) {
    const char c = *begin;
    /* Convert \r to \n. */
    if (c != '\r') {
      if (buffer_file_put (&text_out_buffer, c) != 0) {
	exit ();
      }
    }
    else {
      if (buffer_file_put (&text_out_buffer, '\n') != 0) {
	exit ();
      }
    }
  }

  finish_input (bda, bdb);
}

BEGIN_INPUT (NO_PARAMETER, MOUSE_PACKETS_IN_TERM_NO, "mouse_packets_in_term", "mouse_packet_list_t", mouse_packets_in_term, ano_t ano, int param, bd_t bda, bd_t bdb)
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
    
    if (mouse_packet_list_write (&mouse_packet_list, &mp) != 0) {
      exit ();
    }
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

static bool
mouse_packets_out_precondition (void)
{
  return mouse_packet_list.count != 0;
}

BEGIN_OUTPUT (PARAMETER, MOUSE_PACKETS_OUT_NO, "mouse_packets_out", "mouse_packet_list_t", mouse_packets_out, ano_t ano, int param)
{
  initialize ();

  if (mouse_packets_out_precondition ()) {
    if (mouse_packet_list_reset (&mouse_packet_list) != 0) {
      exit ();
    }
    finish_output (true, mouse_packets_bd, -1);
  }

  finish_output (false, -1, -1);
}

BEGIN_INPUT (NO_PARAMETER, TEXT_IN_NO, "text_in", "buffer_file_t", text_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) != 0) {
    finish_input (bda, bdb);
  }
  
  size_t size = buffer_file_size (&input_buffer);
  const char* data = buffer_file_readp (&input_buffer, size);
  if (data == 0) {
    finish_input (bda, bdb);
  }
  
  const char* begin = data;
  const char* end = data + size;

  for (; begin != end; ++begin) {
    const char c = *begin;
    /* Convert \n to \r\n. */
    if (c == '\n') {
      if (buffer_file_put (&text_out_term_buffer, '\r') != 0) {
	exit ();
      }
    }
    if (buffer_file_put (&text_out_term_buffer, c) != 0) {
      exit ();
    }
  }

  finish_input (bda, bdb);
}

static bool
text_out_term_precondition (void)
{
  return buffer_file_size (&text_out_term_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, TEXT_OUT_TERM_NO, "text_out_term", "buffer_file_t", text_out_term, ano_t ano, int param)
{
  initialize ();

  if (text_out_term_precondition ()) {
    buffer_file_truncate (&text_out_term_buffer);
    finish_output (true, text_out_term_bd, -1);
  }

  finish_output (false, -1, -1);
}

void
do_schedule (void)
{
  if (text_out_precondition ()) {
    schedule (TEXT_OUT_NO, 0);
  }
  if (mouse_packets_out_precondition ()) {
    schedule (MOUSE_PACKETS_OUT_NO, 0);
  }
  if (text_out_term_precondition ()) {
    schedule (TEXT_OUT_TERM_NO, 0);
  }
}
