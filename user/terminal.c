#include <automaton.h>
#include <buffer_file.h>
#include "vga_msg.h"
#include "mouse_msg.h"

/*
  Terminal
  ========

  Routes inputs and outputs from a terminal server.

  Authors
  -------
  Justin R. Wilson
*/

#define INIT_NO 1

#define SCAN_CODES_IN_NO 2
#define STDIN_T_NO 3
#define MOUSE_PACKETS_IN_NO 4

#define SCAN_CODES_OUT_NO 5
#define STDOUT_NO 6
#define MOUSE_PACKETS_OUT_NO 7

#define STDIN_NO 8

#define STDOUT_T_NO 9

/* Initialization flag. */
static bool initialized = false;

/* Keyboard. */
static bd_t scan_codes_bd;
static buffer_file_t scan_codes_buffer;
static bd_t stdout_bd;
static buffer_file_t stdout_buffer;

/* Mouse. */
static bd_t mouse_packets_bd;
static mouse_packet_list_t mouse_packet_list;

static bd_t stdout_t_bd;
static buffer_file_t stdout_t_buffer;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    scan_codes_bd = buffer_create (0);
    if (scan_codes_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&scan_codes_buffer, scan_codes_bd) != 0) {
      exit ();
    }
    
    stdout_bd = buffer_create (0);
    if (stdout_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&stdout_buffer, stdout_bd) != 0) {
      exit ();
    }
    
    mouse_packets_bd = buffer_create (0);
    if (mouse_packets_bd == -1) {
      exit ();
    }
    if (mouse_packet_list_initw (&mouse_packet_list, mouse_packets_bd) != 0) {
      exit ();
    }

    stdout_t_bd = buffer_create (0);
    if (stdout_t_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&stdout_t_buffer, stdout_t_bd) != 0) {
      exit ();
    }

  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

/*
  TERMINAL SERVER -> TERMINAL
*/

/* scan_codes_in
   -------------
   Receive scan codes from the terminal server.
   
   Post: scan codes buffer is not empty
 */
BEGIN_INPUT (NO_PARAMETER, SCAN_CODES_IN_NO, "scan_codes_in_t", "buffer_file_t", scan_codes_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) != 0) {
    finish_input (bda, bdb);
  }
  
  size_t size = buffer_file_size (&input_buffer);
  const unsigned char* codes = buffer_file_readp (&input_buffer, size);
  if (codes == 0) {
    finish_input (bda, bdb);
  }
  
  if (buffer_file_write (&scan_codes_buffer, codes, size) != 0) {
    finish_input (bda, bdb);
  }

  finish_input (bda, bdb);
}

/* stdin_t
   -------
   Receive input from the terminal server.
   
   Post: scan codes buffer is not empty
 */
BEGIN_INPUT (NO_PARAMETER, STDIN_T_NO, "stdin_t", "buffer_file_t", stdin_t, ano_t ano, int param, bd_t bda, bd_t bdb)
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
  
  if (buffer_file_write (&stdout_buffer, data, size) != 0) {
    finish_input (bda, bdb);
  }

  finish_input (bda, bdb);
}

/* mouse_packets_in
   ----------------
   Receive mouse packets from the terminal server.
   
   Post: mouse packet list is not empty
 */
BEGIN_INPUT (NO_PARAMETER, MOUSE_PACKETS_IN_NO, "mouse_packets_in_t", "mouse_packet_list_t", mouse_packets_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  size_t count = 0;
  mouse_packet_t packet;
  mouse_packet_list_t packet_list;
  if (mouse_packet_list_initr (&packet_list, bda, &count) != 0) {
    finish_input (bda, bdb);
  }
  
  for (size_t idx = 0; idx != count; ++idx) {
    if (mouse_packet_list_read (&packet_list, &packet) != 0) {
      finish_input (bda, bdb);
    }
    
    if (mouse_packet_list_write (&mouse_packet_list, &packet) != 0) {
      finish_input (bda, bdb);
    }
  }
  
  finish_input (bda, bdb);
}

/*
  TERMINAL -> CLIENT
*/

static bool
scan_codes_out_precondition (void)
{
  return buffer_file_size (&scan_codes_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, SCAN_CODES_OUT_NO, "scan_codes_out", "buffer_file_t", scan_codes_out, ano_t ano, int param)
{
  initialize ();

  if (scan_codes_out_precondition ()) {
    buffer_file_truncate (&scan_codes_buffer);
    finish_output (true, scan_codes_bd, -1);
  }

  finish_output (false, -1, -1);
}

static bool
stdout_precondition (void)
{
  return buffer_file_size (&stdout_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, STDOUT_NO, "stdout", "buffer_file_t", stdout, int param, aid_t aid)
{
  initialize ();

  if (stdout_precondition ()) {
    buffer_file_truncate (&stdout_buffer);
    finish_output (true, stdout_bd, -1);
  }

  finish_output (false, -1, -1);
}

static bool
mouse_packets_out_precondition (void)
{
  return mouse_packet_list.count != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, MOUSE_PACKETS_OUT_NO, "mouse_packets_out", "mouse_packet_list_t", mouse_packets_out, int param, aid_t aid)
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

/*
  CLIENT -> TERMINAL
*/

/* stdin
   -----
   Receive output from the client.
   
   Post: stdout_t is not empty
 */
BEGIN_INPUT (NO_PARAMETER, STDIN_NO, "stdin", "buffer_file_t", stdin, ano_t ano, int param, bd_t bda, bd_t bdb)
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
  
  if (buffer_file_write (&stdout_t_buffer, data, size) != 0) {
    finish_input (bda, bdb);
  }

  finish_input (bda, bdb);
}

/*
  TERMINAL -> OUTPUT DEVICES
*/

static bool
stdout_t_precondition (void)
{
  return buffer_file_size (&stdout_t_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, STDOUT_T_NO, "stdout_t", "buffer_file_t", stdout_t, int param, aid_t aid)
{
  initialize ();

  if (stdout_t_precondition ()) {
    buffer_file_truncate (&stdout_t_buffer);
    finish_output (true, stdout_t_bd, -1);
  }

  finish_output (false, -1, -1);
}

void
do_schedule (void)
{
  if (scan_codes_out_precondition ()) {
    schedule (SCAN_CODES_OUT_NO, 0);
  }
  if (stdout_precondition ()) {
    schedule (STDOUT_NO, 0);
  }
  if (mouse_packets_out_precondition ()) {
    schedule (MOUSE_PACKETS_OUT_NO, 0);
  }
  if (stdout_t_precondition ()) {
    schedule (STDOUT_T_NO, 0);
  }
}
