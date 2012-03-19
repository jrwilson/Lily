#include <automaton.h>
#include <io.h>
#include <dymem.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <description.h>
#include "vga_msg.h"

#define INIT_NO 1
#define STOP_NO 2
#define SYSLOG_NO 3
#define VGA_OP_NO 4

/* General or external registers. */
#define MISCELLANEOUS_OUTPUT_PORT_READ 0x3CC
#define MISCELLANEOUS_OUTPUT_PORT_WRITE 0x3C2
#define FEATURE_CONTROL_PORT_READ 0x3CA
/* Assuming color. */
#define FEATURE_CONTROL_PORT_WRITE 0x3DA

/* Sequencer registers. */
#define SEQUENCER_ADDRESS_PORT 0x3C4
#define SEQUENCER_DATA_PORT 0x3C5

#define RESET_REGISTER 0
#define CLOCKING_MODE_REGISTER 1
#define MAP_MASK_REGISTER 2
#define CHARACTER_MAP_SELECT_REGISTER 3
#define MEMORY_MODE_REGISTER 4

/* CRT controller registers. */
/* Assuming color. */
#define CRT_ADDRESS_PORT 0x3D4
#define CRT_DATA_PORT 0x3D5

#define HORIZONTAL_TOTAL_REGISTER 0
#define HORIZONTAL_DISPLAY_END_REGISTER 1
#define START_HORIZONTAL_BLANK_REGISTER 2
#define END_HORIZONTAL_BLANK_REGISTER 3
#define START_HORIZONTAL_RETRACE_REGISTER 4
#define END_HORIZONTAL_RETRACE_REGISTER 5
#define VERTICAL_TOTAL_REGISTER 6
#define OVERFLOW_REGISTER 7
#define PRESET_ROW_SCAN_REGISTER 8
#define MAX_SCAN_LINE_REGISTER 9
#define CURSOR_START_REGISTER 10
#define CURSOR_END_REGISTER 11
#define START_ADDRESS_HIGH_REGISTER 12
#define START_ADDRESS_LOW_REGISTER 13
#define CURSOR_LOCATION_HIGH_REGISTER 14
#define CURSOR_LOCATION_LOW_REGISTER 15
#define VERTICAL_RETRACE_START_REGISTER 16
#define VERTICAL_RETRACE_LOW_REGISTER 17
#define VERTICAL_DISPLAY_END_REGISTER 18
#define OFFSET_REGISTER 19
#define UNDERLINE_LOCATION_REGISTER 20
#define START_VERTICAL_BLANK_REGISTER 21
#define END_VERTICAL_BLANK_REGISTER 22
#define MODE_CONTROL_REGISTER 23
#define LINE_COMPARE_REGISTER 24

/* Graphics controller registers. */
#define GRAPHICS_ADDRESS_PORT 0x3CE
#define GRAPHICS_DATA_PORT 0x3CF

#define SET_RESET_REGISTER 0
#define ENABLE_SET_RESET_REGISTER 1
#define COLOR_COMPARE_REGISTER 2
#define DATA_ROTATE_REGISTER 3
#define READ_MAP_SELECT_REGISTER 4
#define MODE_REGISTER 5
#define MISCELLANEOUS_REGISTER 6
#define COLOR_DONT_CARE_REGISTER 7
#define BITMASK_REGISTER 8

/* typedef struct { */
/*   /\* General registers. *\/ */
/*   unsigned char miscellaneous_output; */
/*   unsigned char feature_control; */

/*   /\* Sequencer registers. *\/ */
/*   unsigned char reset; */
/*   unsigned char clocking_mode; */
/*   unsigned char map_mask; */
/*   unsigned char character_map_select; */
/*   unsigned char memory_mode; */

/*   /\* CRT registers. *\/ */
/*   unsigned char horizontal_total; */
/*   unsigned char horizontal_display_end; */
/*   unsigned char start_horizontal_blank; */
/*   unsigned char end_horizontal_blank; */
/*   unsigned char start_horizontal_retrace; */
/*   unsigned char end_horizontal_retrace; */
/*   unsigned char vertical_total; */
/*   union { */
/*     unsigned char overflow_byte; */
/*     struct { */
/*       unsigned char vertical_total_8 : 1; */
/*       unsigned char vertical_display_enable_8 : 1; */
/*       unsigned char vertical_retrace_start_8 : 1; */
/*       unsigned char start_vertical_blanking_8 : 1; */
/*       unsigned char line_compare_8 : 1; */
/*       unsigned char vertical_total_9 : 1; */
/*       unsigned char vertical_display_enable_end_9 : 1; */
/*       unsigned char vertical_retrace_start_9 : 1; */
/*     } overflow; */
/*   } overflow; */
/*   unsigned char preset_row_scan; */
/*   union { */
/*     unsigned char max_scan_line_byte; */
/*     struct { */
/*       unsigned char max_scan_line : 5; */
/*       unsigned char vbs : 1; */
/*       unsigned char line_compare_9 : 1; */
/*       unsigned char two_to_four : 1; */
/*     } max_scan_line; */
/*   } max_scan_line; */
/*   unsigned char cursor_start; */
/*   unsigned char cursor_end; */
/*   unsigned char start_address_high; */
/*   unsigned char start_address_low; */
/*   unsigned char cursor_location_high; */
/*   unsigned char cursor_location_low; */
/*   unsigned char vertical_retrace_start; */
/*   unsigned char vertical_retrace_low; */
/*   unsigned char vertical_display_end; */
/*   unsigned char offset; */
/*   unsigned char underline_location; */
/*   unsigned char start_vertical_blank; */
/*   unsigned char end_vertical_blank; */
/*   unsigned char mode_control; */
/*   unsigned char line_compare; */

/*   /\* Graphics registers. *\/ */
/*   unsigned char set_reset; */
/*   unsigned char enable_set_reset; */
/*   unsigned char color_compare; */
/*   unsigned char data_rotate; */
/*   unsigned char read_map_select; */
/*   unsigned char mode; */
/*   unsigned char miscellaneous; */
/*   unsigned char color_dont_care; */
/*   unsigned char bitmask; */
/* } vga_registers_t; */

/* static void */
/* read_vga_registers (vga_registers_t* r) */
/* { */
/*   r->miscellaneous_output = inb (MISCELLANEOUS_OUTPUT_PORT_READ); */
/*   r->feature_control = inb (FEATURE_CONTROL_PORT_READ); */

/*   outb (SEQUENCER_ADDRESS_PORT, RESET_REGISTER); */
/*   r->reset = inb (SEQUENCER_DATA_PORT); */
/*   outb (SEQUENCER_ADDRESS_PORT, CLOCKING_MODE_REGISTER); */
/*   r->clocking_mode = inb (SEQUENCER_DATA_PORT); */
/*   outb (SEQUENCER_ADDRESS_PORT, MAP_MASK_REGISTER); */
/*   r->map_mask = inb (SEQUENCER_DATA_PORT); */
/*   outb (SEQUENCER_ADDRESS_PORT, CHARACTER_MAP_SELECT_REGISTER); */
/*   r->character_map_select = inb (SEQUENCER_DATA_PORT); */
/*   outb (SEQUENCER_ADDRESS_PORT, MEMORY_MODE_REGISTER); */
/*   r->memory_mode = inb (SEQUENCER_DATA_PORT); */

/*   outb (CRT_ADDRESS_PORT, HORIZONTAL_TOTAL_REGISTER); */
/*   r->horizontal_total = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, HORIZONTAL_DISPLAY_END_REGISTER); */
/*   r->horizontal_display_end = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, START_HORIZONTAL_BLANK_REGISTER); */
/*   r->start_horizontal_blank = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, END_HORIZONTAL_BLANK_REGISTER); */
/*   r->end_horizontal_blank = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, START_HORIZONTAL_RETRACE_REGISTER); */
/*   r->start_horizontal_retrace = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, END_HORIZONTAL_RETRACE_REGISTER); */
/*   r->end_horizontal_retrace = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, VERTICAL_TOTAL_REGISTER); */
/*   r->vertical_total = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, OVERFLOW_REGISTER); */
/*   r->overflow.overflow_byte = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, PRESET_ROW_SCAN_REGISTER); */
/*   r->preset_row_scan = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, MAX_SCAN_LINE_REGISTER); */
/*   r->max_scan_line.max_scan_line_byte = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, CURSOR_START_REGISTER); */
/*   r->cursor_start = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, CURSOR_END_REGISTER); */
/*   r->cursor_end = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, START_ADDRESS_HIGH_REGISTER); */
/*   r->start_address_high = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, START_ADDRESS_LOW_REGISTER); */
/*   r->start_address_low = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_HIGH_REGISTER); */
/*   r->cursor_location_high = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_LOW_REGISTER); */
/*   r->cursor_location_low = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, VERTICAL_RETRACE_START_REGISTER); */
/*   r->vertical_retrace_start = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, VERTICAL_RETRACE_LOW_REGISTER); */
/*   r->vertical_retrace_low = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, VERTICAL_DISPLAY_END_REGISTER); */
/*   r->vertical_display_end = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, OFFSET_REGISTER); */
/*   r->offset = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, UNDERLINE_LOCATION_REGISTER); */
/*   r->underline_location = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, START_VERTICAL_BLANK_REGISTER); */
/*   r->start_vertical_blank = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, END_VERTICAL_BLANK_REGISTER); */
/*   r->end_vertical_blank = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, MODE_CONTROL_REGISTER); */
/*   r->mode_control = inb (CRT_DATA_PORT); */
/*   outb (CRT_ADDRESS_PORT, LINE_COMPARE_REGISTER); */
/*   r->line_compare = inb (CRT_DATA_PORT); */

/*   outb (GRAPHICS_ADDRESS_PORT, SET_RESET_REGISTER); */
/*   r->set_reset = inb (GRAPHICS_DATA_PORT); */
/*   outb (GRAPHICS_ADDRESS_PORT, ENABLE_SET_RESET_REGISTER); */
/*   r->enable_set_reset = inb (GRAPHICS_DATA_PORT); */
/*   outb (GRAPHICS_ADDRESS_PORT, COLOR_COMPARE_REGISTER); */
/*   r->color_compare = inb (GRAPHICS_DATA_PORT); */
/*   outb (GRAPHICS_ADDRESS_PORT, DATA_ROTATE_REGISTER); */
/*   r->data_rotate = inb (GRAPHICS_DATA_PORT); */
/*   outb (GRAPHICS_ADDRESS_PORT, READ_MAP_SELECT_REGISTER); */
/*   r->read_map_select = inb (GRAPHICS_DATA_PORT); */
/*   outb (GRAPHICS_ADDRESS_PORT, MODE_REGISTER); */
/*   r->mode = inb (GRAPHICS_DATA_PORT); */
/*   outb (GRAPHICS_ADDRESS_PORT, MISCELLANEOUS_REGISTER); */
/*   r->miscellaneous = inb (GRAPHICS_DATA_PORT); */
/*   outb (GRAPHICS_ADDRESS_PORT, COLOR_DONT_CARE_REGISTER); */
/*   r->color_dont_care = inb (GRAPHICS_DATA_PORT); */
/*   outb (GRAPHICS_ADDRESS_PORT, BITMASK_REGISTER); */
/*   r->bitmask = inb (GRAPHICS_DATA_PORT); */
/* } */

/* static void */
/* write_vga_registers (const vga_registers_t* r) */
/* { */
/*   outb (MISCELLANEOUS_OUTPUT_PORT_WRITE, r->miscellaneous_output); */
/*   outb (FEATURE_CONTROL_PORT_WRITE, r->feature_control); */

/*   outb (SEQUENCER_ADDRESS_PORT, RESET_REGISTER); */
/*   outb (SEQUENCER_DATA_PORT, r->reset); */
/*   outb (SEQUENCER_ADDRESS_PORT, CLOCKING_MODE_REGISTER); */
/*   outb (SEQUENCER_DATA_PORT, r->clocking_mode); */
/*   outb (SEQUENCER_ADDRESS_PORT, MAP_MASK_REGISTER); */
/*   outb (SEQUENCER_DATA_PORT, r->map_mask); */
/*   outb (SEQUENCER_ADDRESS_PORT, CHARACTER_MAP_SELECT_REGISTER); */
/*   outb (SEQUENCER_DATA_PORT, r->character_map_select); */
/*   outb (SEQUENCER_ADDRESS_PORT, MEMORY_MODE_REGISTER); */
/*   outb (SEQUENCER_DATA_PORT, r->memory_mode); */

/*   outb (CRT_ADDRESS_PORT, HORIZONTAL_TOTAL_REGISTER); */
/*   outb (CRT_DATA_PORT, r->horizontal_total); */
/*   outb (CRT_ADDRESS_PORT, HORIZONTAL_DISPLAY_END_REGISTER); */
/*   outb (CRT_DATA_PORT, r->horizontal_display_end); */
/*   outb (CRT_ADDRESS_PORT, START_HORIZONTAL_BLANK_REGISTER); */
/*   outb (CRT_DATA_PORT, r->start_horizontal_blank); */
/*   outb (CRT_ADDRESS_PORT, END_HORIZONTAL_BLANK_REGISTER); */
/*   outb (CRT_DATA_PORT, r->end_horizontal_blank); */
/*   outb (CRT_ADDRESS_PORT, START_HORIZONTAL_RETRACE_REGISTER); */
/*   outb (CRT_DATA_PORT, r->start_horizontal_retrace); */
/*   outb (CRT_ADDRESS_PORT, END_HORIZONTAL_RETRACE_REGISTER); */
/*   outb (CRT_DATA_PORT, r->end_horizontal_retrace); */
/*   outb (CRT_ADDRESS_PORT, VERTICAL_TOTAL_REGISTER); */
/*   outb (CRT_DATA_PORT, r->vertical_total); */
/*   outb (CRT_ADDRESS_PORT, OVERFLOW_REGISTER); */
/*   outb (CRT_DATA_PORT, r->overflow.overflow_byte); */
/*   outb (CRT_ADDRESS_PORT, PRESET_ROW_SCAN_REGISTER); */
/*   outb (CRT_DATA_PORT, r->preset_row_scan); */
/*   outb (CRT_ADDRESS_PORT, MAX_SCAN_LINE_REGISTER); */
/*   outb (CRT_DATA_PORT, r->max_scan_line.max_scan_line_byte); */
/*   outb (CRT_ADDRESS_PORT, CURSOR_START_REGISTER); */
/*   outb (CRT_DATA_PORT, r->cursor_start); */
/*   outb (CRT_ADDRESS_PORT, CURSOR_END_REGISTER); */
/*   outb (CRT_DATA_PORT, r->cursor_end); */
/*   outb (CRT_ADDRESS_PORT, START_ADDRESS_HIGH_REGISTER); */
/*   outb (CRT_DATA_PORT, r->start_address_high); */
/*   outb (CRT_ADDRESS_PORT, START_ADDRESS_LOW_REGISTER); */
/*   outb (CRT_DATA_PORT, r->start_address_low); */
/*   outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_HIGH_REGISTER); */
/*   outb (CRT_DATA_PORT, r->cursor_location_high); */
/*   outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_LOW_REGISTER); */
/*   outb (CRT_DATA_PORT, r->cursor_location_low); */
/*   outb (CRT_ADDRESS_PORT, VERTICAL_RETRACE_START_REGISTER); */
/*   outb (CRT_DATA_PORT, r->vertical_retrace_start); */
/*   outb (CRT_ADDRESS_PORT, VERTICAL_RETRACE_LOW_REGISTER); */
/*   outb (CRT_DATA_PORT, r->vertical_retrace_low); */
/*   outb (CRT_ADDRESS_PORT, VERTICAL_DISPLAY_END_REGISTER); */
/*   outb (CRT_DATA_PORT, r->vertical_display_end); */
/*   outb (CRT_ADDRESS_PORT, OFFSET_REGISTER); */
/*   outb (CRT_DATA_PORT, r->offset); */
/*   outb (CRT_ADDRESS_PORT, UNDERLINE_LOCATION_REGISTER); */
/*   outb (CRT_DATA_PORT, r->underline_location); */
/*   outb (CRT_ADDRESS_PORT, START_VERTICAL_BLANK_REGISTER); */
/*   outb (CRT_DATA_PORT, r->start_vertical_blank); */
/*   outb (CRT_ADDRESS_PORT, END_VERTICAL_BLANK_REGISTER); */
/*   outb (CRT_DATA_PORT, r->end_vertical_blank); */
/*   outb (CRT_ADDRESS_PORT, MODE_CONTROL_REGISTER); */
/*   outb (CRT_DATA_PORT, r->mode_control); */
/*   outb (CRT_ADDRESS_PORT, LINE_COMPARE_REGISTER); */
/*   outb (CRT_DATA_PORT, r->line_compare); */

/*   outb (GRAPHICS_ADDRESS_PORT, SET_RESET_REGISTER); */
/*   outb (GRAPHICS_DATA_PORT, r->set_reset); */
/*   outb (GRAPHICS_ADDRESS_PORT, ENABLE_SET_RESET_REGISTER); */
/*   outb (GRAPHICS_DATA_PORT, r->enable_set_reset); */
/*   outb (GRAPHICS_ADDRESS_PORT, COLOR_COMPARE_REGISTER); */
/*   outb (GRAPHICS_DATA_PORT, r->color_compare); */
/*   outb (GRAPHICS_ADDRESS_PORT, DATA_ROTATE_REGISTER); */
/*   outb (GRAPHICS_DATA_PORT, r->data_rotate); */
/*   outb (GRAPHICS_ADDRESS_PORT, READ_MAP_SELECT_REGISTER); */
/*   outb (GRAPHICS_DATA_PORT, r->read_map_select); */
/*   outb (GRAPHICS_ADDRESS_PORT, MODE_REGISTER); */
/*   outb (GRAPHICS_DATA_PORT, r->mode); */
/*   outb (GRAPHICS_ADDRESS_PORT, MISCELLANEOUS_REGISTER); */
/*   outb (GRAPHICS_DATA_PORT, r->miscellaneous); */
/*   outb (GRAPHICS_ADDRESS_PORT, COLOR_DONT_CARE_REGISTER); */
/*   outb (GRAPHICS_DATA_PORT, r->color_dont_care); */
/*   outb (GRAPHICS_ADDRESS_PORT, BITMASK_REGISTER); */
/*   outb (GRAPHICS_DATA_PORT, r->bitmask); */
/* } */

/* Initialization flag. */
static bool initialized = false;

#define NAME "vga"
#define SYSLOG_NAME "syslog"
#define SYSLOG_STDIN "stdin"

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

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;
    
    /* Create the syslog buffer. */
    syslog_bd = buffer_create (0);
    if (syslog_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&syslog_buffer, syslog_bd) != 0) {
      exit ();
    }

    aid_t syslog_aid = lookup (SYSLOG_NAME, strlen (SYSLOG_NAME) + 1);
    if (syslog_aid != -1) {
      /* Bind to the syslog. */

      description_t syslog_description;
      if (description_init (&syslog_description, syslog_aid) != 0) {
	exit ();
      }
      
      const ano_t syslog_stdin = description_name_to_number (&syslog_description, SYSLOG_STDIN, strlen (SYSLOG_STDIN) + 1);
      
      description_fini (&syslog_description);
      
      /* We bind the response first so they don't get lost. */
      if (bind (getaid (), SYSLOG_NO, 0, syslog_aid, syslog_stdin, 0) == -1) {
	exit ();
      }
    }

    /* Reserve all of the VGA ports.*/
    if (reserve_port (MISCELLANEOUS_OUTPUT_PORT_READ) != 0 ||
	reserve_port (MISCELLANEOUS_OUTPUT_PORT_WRITE) != 0 ||
	reserve_port (FEATURE_CONTROL_PORT_READ) != 0 ||
	reserve_port (FEATURE_CONTROL_PORT_WRITE) != 0 ||
	reserve_port (SEQUENCER_ADDRESS_PORT) != 0 ||
	reserve_port (SEQUENCER_DATA_PORT) != 0 ||
	reserve_port (CRT_ADDRESS_PORT) != 0 ||
	reserve_port (CRT_DATA_PORT) != 0 ||
	reserve_port (GRAPHICS_ADDRESS_PORT) != 0 ||
	reserve_port (GRAPHICS_DATA_PORT) != 0) {
      bfprintf (&syslog_buffer, "%s: error: could not reserve I/O ports\n", NAME);
      state = STOP;
      return;
    }

    /* Map in the video memory. */
    if (map ((const void*)VGA_VIDEO_MEMORY_BEGIN, (const void*)VGA_VIDEO_MEMORY_BEGIN, VGA_VIDEO_MEMORY_SIZE) != 0) {
      bfprintf (&syslog_buffer, "%s: error: could not map vga video memory\n", NAME);
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
  end_internal_action ();
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
  scheduler_remove (ano, param);

  if (stop_precondition ()) {
    exit ();
  }
  end_internal_action ();
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
  scheduler_remove (ano, param);

  if (syslog_precondition ()) {
    buffer_file_truncate (&syslog_buffer);
    end_output_action (true, syslog_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

BEGIN_INPUT (NO_PARAMETER, VGA_OP_NO, "vga_op", "vga_op_list", vga_op, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (state == RUN) {
    vga_op_list_t vol;
    size_t count;
    if (vga_op_list_initr (&vol, bda, bdb, &count) != 0) {
      end_input_action (bda, bdb);
    }
    
    for (size_t i = 0; i != count; ++i) {
      vga_op_type_t type;
      if (vga_op_list_next_op_type (&vol, &type) != 0) {
	end_input_action (bda, bdb);
      }
      
      switch (type) {
      case VGA_SET_CURSOR_LOCATION:
	{
	  size_t location;
	  if (vga_op_list_read_set_cursor_location (&vol, &location) != 0) {
	    end_input_action (bda, bdb);
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
	    end_input_action (bda, bdb);
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
	    end_input_action (bda, bdb);
	  }
	  assign (address, data, size);
	}
	break;
      default:
	end_input_action (bda, bdb);
      }
    }
  }

  end_input_action (bda, bdb);
}

void
schedule (void)
{
  if (stop_precondition ()) {
    scheduler_add (STOP_NO, 0);
  }
  if (syslog_precondition ()) {
    scheduler_add (SYSLOG_NO, 0);
  }
}
