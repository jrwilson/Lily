#include <automaton.h>
#include <dymem.h>
#include <string.h>
#include <description.h>
#include "vga_msg.h"
#include "syslog.h"

#define INIT_NO 1
#define STOP_NO 2
#define SYSLOG_NO 3
#define VGA_OP_NO 4

/* Assuming color. */
#define CRT_ADDRESS_PORT 0x3D4
#define CRT_DATA_PORT 0x3D5
/* #define HORIZONTAL_TOTAL_REGISTER 0 */
/* #define HORIZONTAL_DISPLAY_END_REGISTER 1 */
/* #define START_HORIZONTAL_BLANK_REGISTER 2 */
/* #define END_HORIZONTAL_BLANK_REGISTER 3 */
/* #define START_HORIZONTAL_RETRACE_REGISTER 4 */
/* #define END_HORIZONTAL_RETRACE_REGISTER 5 */
/* #define VERTICAL_TOTAL_REGISTER 6 */
/* #define OVERFLOW_REGISTER 7 */
/* #define PRESET_ROW_SCAN_REGISTER 8 */
/* #define MAX_SCAN_LINE_REGISTER 9 */
/* #define CURSOR_START_REGISTER 10 */
/* #define CURSOR_END_REGISTER 11 */
/* #define START_ADDRESS_HIGH_REGISTER 12 */
/* #define START_ADDRESS_LOW_REGISTER 13 */
#define CURSOR_LOCATION_HIGH_REGISTER 14
#define CURSOR_LOCATION_LOW_REGISTER 15
/* #define VERTICAL_RETRACE_START_REGISTER 16 */
/* #define VERTICAL_RETRACE_LOW_REGISTER 17 */
/* #define VERTICAL_DISPLAY_END_REGISTER 18 */
/* #define OFFSET_REGISTER 19 */
/* #define UNDERLINE_LOCATION_REGISTER 20 */
/* #define START_VERTICAL_BLANK_REGISTER 21 */
/* #define END_VERTICAL_BLANK_REGISTER 22 */
/* #define MODE_CONTROL_REGISTER 23 */
/* #define LINE_COMPARE_REGISTER 24 */

/* Initialization flag. */
static bool initialized = false;

#define ERROR "vga: error: "
#define INFO "info: error: "

typedef enum {
  RUN,
  STOP,
} state_t;
static state_t state = RUN;

static bd_t syslog_bd = -1;
static buffer_file_t syslog_buffer;

static void
set_cursor_location (unsigned short location)
{
  outb (0, CRT_ADDRESS_PORT, CURSOR_LOCATION_HIGH_REGISTER);
  outb (0, CRT_DATA_PORT, location >> 8);
  outb (0, CRT_ADDRESS_PORT, CURSOR_LOCATION_LOW_REGISTER);
  outb (0, CRT_DATA_PORT, location & 0xFF);
}

static void
assign (size_t address,
	const void* data,
	size_t size)
{
  if (address >= VGA_VIDEO_MEMORY_BEGIN &&
      address + size < VGA_VIDEO_MEMORY_END) {
    memcpy ((void*)address, data, size);
  }
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;
    
    /* Create the syslog buffer. */
    syslog_bd = buffer_create (0, 0);
    if (syslog_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&syslog_buffer, 0, syslog_bd) != 0) {
      exit ();
    }

    aid_t syslog_aid = lookups (0, SYSLOG_NAME);
    if (syslog_aid != -1) {
      /* Bind to the syslog. */

      description_t syslog_description;
      if (description_init (&syslog_description, 0, syslog_aid) != 0) {
	exit ();
      }
      
      action_desc_t syslog_text_in;
      if (description_read_name (&syslog_description, &syslog_text_in, SYSLOG_TEXT_IN) != 0) {
	exit ();
      }
            
      /* We bind the response first so they don't get lost. */
      if (bind (0, getaid (), SYSLOG_NO, 0, syslog_aid, syslog_text_in.number, 0) == -1) {
	exit ();
      }

      description_fini (&syslog_description, 0);
    }

    /* Reserve all of the VGA ports.*/
    if (reserve_port (0, CRT_ADDRESS_PORT) != 0 ||
	reserve_port (0, CRT_DATA_PORT) != 0) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not reserve I/O ports\n");
      state = STOP;
      return;
    }

    /* Map in the video memory. */
    if (map (0, (const void*)VGA_VIDEO_MEMORY_BEGIN, (const void*)VGA_VIDEO_MEMORY_BEGIN, VGA_VIDEO_MEMORY_SIZE) != 0) {
      buffer_file_puts (&syslog_buffer, 0, ERROR "could not map vga video memory\n");
      state = STOP;
      return;
    }
    /* Clear the text region. */
    memset ((void*)VGA_TEXT_MEMORY_BEGIN, 0, VGA_TEXT_MEMORY_SIZE);
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

/* stop
   ----
   Stop the automaton.
   
   Pre:  state == STOP and syslog_buffer is empty
   Post: 
*/
static bool
stop_precondition (void)
{
  return state == STOP && buffer_file_size (&syslog_buffer) == 0;
}

BEGIN_INTERNAL (NO_PARAMETER, STOP_NO, "", "", stop, ano_t ano, int param)
{
  initialize ();

  if (stop_precondition ()) {
    exit ();
  }
  finish_internal ();
}

/* syslog
   ------
   Output error messages.
   
   Pre:  syslog_buffer is not empty
   Post: syslog_buffer is empty
*/
static bool
syslog_precondition (void)
{
  return buffer_file_size (&syslog_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, SYSLOG_NO, "", "", syslogx, ano_t ano, int param)
{
  initialize ();

  if (syslog_precondition ()) {
    buffer_file_truncate (&syslog_buffer);
    finish_output (true, syslog_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

BEGIN_INPUT (NO_PARAMETER, VGA_OP_NO, "vga_op_in", "vga_op_list", vga_op, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (state == RUN) {
    vga_op_list_t vol;
    if (vga_op_list_initr (&vol, 0, bda, bdb) != 0) {
      finish_input (bda, bdb);
    }
    
    for (size_t i = 0; i != vol.count; ++i) {
      vga_op_type_t type;
      if (vga_op_list_next_op_type (&vol, &type) != 0) {
	finish_input (bda, bdb);
      }
      
      switch (type) {
      case VGA_SET_CURSOR_LOCATION:
	{
	  size_t location;
	  if (vga_op_list_read_set_cursor_location (&vol, &location) != 0) {
	    finish_input (bda, bdb);
	  }
	  set_cursor_location (location);
	}
	break;
      case VGA_ASSIGN:
	{
	  size_t address;
	  const void* data;
	  size_t size;
	  if (vga_op_list_read_assign (&vol, &address, &data, &size) != 0) {
	    finish_input (bda, bdb);
	  }
	  assign (address, data, size);
	}
	break;
      case VGA_BASSIGN:
	{
	  size_t address;
	  const void* data;
	  size_t size;
	  if (vga_op_list_read_bassign (&vol, &address, &data, &size) != 0) {
	    finish_input (bda, bdb);
	  }
	  assign (address, data, size);
	}
	break;
      default:
	finish_input (bda, bdb);
      }
    }
  }

  finish_input (bda, bdb);
}

void
do_schedule (void)
{
  if (stop_precondition ()) {
    schedule (0, STOP_NO, 0);
  }
  if (syslog_precondition ()) {
    schedule (0, SYSLOG_NO, 0);
  }
}
