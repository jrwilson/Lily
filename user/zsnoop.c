/*
  zsnoop.c
  ========
  A snooper for the ZMODEM protocol.
  
  Before trying to implement the ZMODEM protocol, I thought it best to passively decode ZMODEM.
  Assume we have the following network:
  +-------+                 +-----+                 +-----+
  | Alice | <- COM1/COM1 -> | Eve | <- COM2/COM1 -> | Bob |
  +-------+                 +-----+                 +-----+
  Eve forwards bytes arriving on COM1 to COM2 and forwards bytes arriving on COM2 to COM1.
  +----------+        +----------+
  |   COM1   |        |   COM2   |
  | text_out | -----> | text_in  |
  | text_in  | <----- | text_out |
  +----------+        +----------+
  We then insert the snooper (other bindings not shown):
  +----------+        +-----------+
  |   COM1   |        | Zsnoop    |
  | text_out | -----> | text_in_a |
  +----------+        |           |
                      | text_out  | -----> (to terminal)
  +----------+        |           |
  |   COM2   |        |           |
  | text_out | -----> | text_in_b |
  +----------+        +-----------+
*/

#include <automaton.h>
#include <buffer_file.h>

#define INIT_NO 1
#define TEXT_IN_A_NO 2
#define TEXT_IN_B_NO 3
#define TEXT_OUT_NO 4

/* Initialization flag. */
static bool initialized = false;

static bd_t text_out_bd = -1;
static buffer_file_t text_out_buffer;

typedef enum {
  HOST_NONE,
  HOST_A,
  HOST_B,
} host_t;
static host_t last_host = HOST_NONE;

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

BEGIN_INPUT (NO_PARAMETER, TEXT_IN_A_NO, "text_in_a", "buffer_file_t", text_in_a, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, 0, bda) != 0) {
    finish_input (bda, bdb);
  }

  size_t size = buffer_file_size (&input_buffer);
  const unsigned char* str = buffer_file_readp (&input_buffer, size);
  if (str == 0) {
    finish_input (bda, bdb);
  }

  switch (last_host) {
  case HOST_NONE:
    buffer_file_puts (&text_out_buffer, 0, "A:");
    break;
  case HOST_A:
    /* Do nothing. */
    break;
  case HOST_B:
    buffer_file_puts (&text_out_buffer, 0, "\nA:");
    break;
  }

  for (size_t idx = 0; idx != size; ++idx) {
    bfprintf (&text_out_buffer, 0, " %x", str[idx]);
  }

  last_host = HOST_A;

  finish_input (bda, bdb);
}

BEGIN_INPUT (NO_PARAMETER, TEXT_IN_B_NO, "text_in_b", "buffer_file_t", text_in_b, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, 0, bda) != 0) {
    finish_input (bda, bdb);
  }

  size_t size = buffer_file_size (&input_buffer);
  const unsigned char* str = buffer_file_readp (&input_buffer, size);
  if (str == 0) {
    finish_input (bda, bdb);
  }

  switch (last_host) {
  case HOST_NONE:
    buffer_file_puts (&text_out_buffer, 0, "B:");
    break;
  case HOST_A:
    buffer_file_puts (&text_out_buffer, 0, "\nB:");
    break;
  case HOST_B:
    /* Do nothing. */
    break;
  }

  for (size_t idx = 0; idx != size; ++idx) {
    bfprintf (&text_out_buffer, 0, " %x", str[idx]);
  }

  last_host = HOST_B;

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
}
