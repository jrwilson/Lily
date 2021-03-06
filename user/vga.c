#include <automaton.h>
#include <dymem.h>
#include <string.h>
#include <description.h>
#include "vga_msg.h"

#define INIT_NO 1
#define VGA_OP_NO 2

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

#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "
#define INFO __FILE__ ": info: "

typedef struct {
  unsigned int ptr;
} __attribute__((packed)) video_parameter_t;

static void
set_cursor_location (unsigned short location)
{
  outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_HIGH_REGISTER);
  outb (CRT_DATA_PORT, location >> 8);
  outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_LOW_REGISTER);
  outb (CRT_DATA_PORT, location & 0xFF);
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

#define USAGE "usage: port=PORT [parameter_table=ADDRESS]\n"

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;
    
    /* bd_t bda = getinita (); */
    /* bd_t bdb = getinitb (); */

    /* if (bda == -1) { */
    /*   buffer_file_puts (&syslog_buffer, 0, ERROR USAGE); */
    /*   state = STOP; */
    /*   return; */
    /* } */

    /* int port = -1; */
    /* unsigned int parameter_table = 0; */

    /* /\* Process arguments. *\/ */
    /* buffer_file_t bf; */
    /* if (buffer_file_initr (&bf, 0, bda) != 0) { */
    /*   buffer_file_puts (&syslog_buffer, 0, ERROR "could not initialize init buffer\n"); */
    /*   state = STOP; */
    /*   return; */
    /* } */

    /* const size_t size = buffer_file_size (&bf); */
    /* const char* begin = buffer_file_readp (&bf, size); */
    /* if (begin == 0) { */
    /*   buffer_file_puts (&syslog_buffer, 0, ERROR "could not read init buffer\n"); */
    /*   state = STOP; */
    /*   return; */
    /* } */
    /* const char* end = begin + size; */
    /* const char* ptr = begin; */

    /* bool error = false; */

    /* char* key = 0; */
    /* char* value = 0; */
    /* while (!error && ptr != end && kv_parse (0, &key, &value, &ptr, end) == 0) { */
    /*   if (key != 0) { */
    /* 	if (strcmp (key, "port") == 0) { */
    /* 	  if (value != 0) { */
    /* 	    string_error_t err; */
    /* 	    char* ptr; */
    /* 	    unsigned short p = strtoul (&err, value, &ptr, 0); */
    /* 	    if (err == STRING_SUCCESS && *ptr == '\0') { */
    /* 	      port = p; */
    /* 	    } */
    /* 	    else { */
    /* 	      bfprintf (&syslog_buffer, ERROR "could not parse port value %s\n", value); */
    /* 	      error = true; */
    /* 	    } */
    /* 	  } */
    /* 	  else { */
    /* 	    buffer_file_puts (&syslog_buffer, 0, ERROR "port requires a value\n"); */
    /* 	    error = true; */
    /* 	  } */
    /* 	} */
    /* 	else if (strcmp (key, "parameter_table") == 0) { */
    /* 	  if (value != 0) { */
    /* 	    string_error_t err; */
    /* 	    char* ptr; */
    /* 	    unsigned int p = strtoul (&err, value, &ptr, 0); */
    /* 	    if (err == STRING_SUCCESS && *ptr == '\0') { */
    /* 	      parameter_table = p; */
    /* 	    } */
    /* 	    else { */
    /* 	      bfprintf (&syslog_buffer, ERROR "could not parse parameter_table value %s\n", value); */
    /* 	      error = true; */
    /* 	    } */
    /* 	  } */
    /* 	  else { */
    /* 	    buffer_file_puts (&syslog_buffer, 0, ERROR "parameter_table requires a value\n"); */
    /* 	    error = true; */
    /* 	  } */
    /* 	} */
    /* 	else { */
    /* 	  bfprintf (&syslog_buffer, ERROR "unknown option %s\n", key); */
    /* 	  error = true; */
    /* 	} */
    /*   } */
    /*   free (key); */
    /*   free (value); */
    /* } */

    /* if (error) { */
    /*   state = STOP; */
    /*   return; */
    /* } */

    /* bfprintf (&syslog_buffer, INFO "parameter_table=%#x\n", parameter_table); */

    /* if (port == -1/\* || parameter_table == 0*\/) { */
    /*   buffer_file_puts (&syslog_buffer, 0, ERROR USAGE); */
    /*   state = STOP; */
    /*   return; */
    /* } */

    /* if (bda != -1) { */
    /*   buffer_destroy (0, bda); */
    /* } */
    /* if (bdb != -1) { */
    /*   buffer_destroy (0, bdb); */
    /* } */

    /* if (port != CRT_ADDRESS_PORT) { */
    /*   buffer_file_puts (&syslog_buffer, 0, ERROR "port %#x not supported\n"); */
    /*   state = STOP; */
    /*   return; */
    /* } */
    
    /* Reserve the VGA ports.*/
    if (reserve_port (CRT_ADDRESS_PORT) != 0 ||
	reserve_port (CRT_DATA_PORT) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not reserve I/O ports: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    /* /\* Map in the parameter table below the video memory. *\/ */
    /* unsigned int parameter_table_begin = parameter_table & 0xFFFFF; */
    /* unsigned int parameter_table_end = parameter_table_begin + 23 * 64; /\* TODO:  Magic numbers. *\/ */
    /* if (!(parameter_table_end <= VGA_VIDEO_MEMORY_BEGIN)) { */
    /*   buffer_file_puts (&syslog_buffer, 0, ERROR "parameter table conflicts with video memory\n"); */
    /*   state = STOP; */
    /*   return; */
    /* } */

    /* if (map (0, (const void*)parameter_table_begin, (const void*)parameter_table, 23 * 64) != 0) { */
    /*   buffer_file_puts (&syslog_buffer, 0, ERROR "could not map parameter table\n"); */
    /*   state = STOP; */
    /*   return; */
    /* } */
    
    /* Map in the video memory. */
    if (map ((const void*)VGA_VIDEO_MEMORY_BEGIN, (const void*)VGA_VIDEO_MEMORY_BEGIN, VGA_VIDEO_MEMORY_SIZE) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not map vga video memory: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    /* Clear the text region. */
    memset ((void*)VGA_TEXT_MEMORY_BEGIN, 0, VGA_TEXT_MEMORY_SIZE);

    /* TODO:  Ensure that a VGA exists. */
    /* bfprintf (&syslog_buffer, INFO "port=%#x\n", port); */

    /* const video_parameter_t* params = (const video_parameter_t*)parameter_table_begin; */
    /* bfprintf (&syslog_buffer, INFO "ptr=%#x\n", params->ptr); */

  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

BEGIN_INPUT (NO_PARAMETER, VGA_OP_NO, "vga_op_in", "vga_op_list", vga_op, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();
  
  vga_op_list_t vol;
  if (vga_op_list_initr (&vol, bda, bdb) != 0) {
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
  
  finish_input (bda, bdb);
}

void
do_schedule (void)
{ }
