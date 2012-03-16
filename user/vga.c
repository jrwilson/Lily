#include <automaton.h>
#include <io.h>
#include <dymem.h>
#include <string.h>
#include "vga_msg.h"

/* TODO:  Error handling. */

#define DESTROYED_NO 1
#define VGA_OP_NO 2
#define INIT_NO 3

#define VIDEO_MEMORY_BEGIN 0xA0000
#define VIDEO_MEMORY_END   0xC0000
#define VIDEO_MEMORY_SIZE (VIDEO_MEMORY_END - VIDEO_MEMORY_BEGIN)

#define TEXT_MEMORY_BEGIN 0xB8000
#define TEXT_MEMORY_END   0xC0000
#define TEXT_MEMORY_SIZE  (TEXT_MEMORY_END - TEXT_MEMORY_BEGIN)

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

typedef struct {
  /* General registers. */
  unsigned char miscellaneous_output;
  unsigned char feature_control;

  /* Sequencer registers. */
  unsigned char reset;
  unsigned char clocking_mode;
  unsigned char map_mask;
  unsigned char character_map_select;
  unsigned char memory_mode;

  /* CRT registers. */
  unsigned char horizontal_total;
  unsigned char horizontal_display_end;
  unsigned char start_horizontal_blank;
  unsigned char end_horizontal_blank;
  unsigned char start_horizontal_retrace;
  unsigned char end_horizontal_retrace;
  unsigned char vertical_total;
  union {
    unsigned char overflow_byte;
    struct {
      unsigned char vertical_total_8 : 1;
      unsigned char vertical_display_enable_8 : 1;
      unsigned char vertical_retrace_start_8 : 1;
      unsigned char start_vertical_blanking_8 : 1;
      unsigned char line_compare_8 : 1;
      unsigned char vertical_total_9 : 1;
      unsigned char vertical_display_enable_end_9 : 1;
      unsigned char vertical_retrace_start_9 : 1;
    } overflow;
  } overflow;
  unsigned char preset_row_scan;
  union {
    unsigned char max_scan_line_byte;
    struct {
      unsigned char max_scan_line : 5;
      unsigned char vbs : 1;
      unsigned char line_compare_9 : 1;
      unsigned char two_to_four : 1;
    } max_scan_line;
  } max_scan_line;
  unsigned char cursor_start;
  unsigned char cursor_end;
  unsigned char start_address_high;
  unsigned char start_address_low;
  unsigned char cursor_location_high;
  unsigned char cursor_location_low;
  unsigned char vertical_retrace_start;
  unsigned char vertical_retrace_low;
  unsigned char vertical_display_end;
  unsigned char offset;
  unsigned char underline_location;
  unsigned char start_vertical_blank;
  unsigned char end_vertical_blank;
  unsigned char mode_control;
  unsigned char line_compare;

  /* Graphics registers. */
  unsigned char set_reset;
  unsigned char enable_set_reset;
  unsigned char color_compare;
  unsigned char data_rotate;
  unsigned char read_map_select;
  unsigned char mode;
  unsigned char miscellaneous;
  unsigned char color_dont_care;
  unsigned char bitmask;
} vga_registers_t;

typedef struct client_context client_context_t;
struct client_context {
  aid_t aid;
  vga_registers_t registers;
  bd_t bd; /* A buffer for video memory. */
  void* buffer; /* Pointer to the buffer if mapped. */
  size_t default_offset;
  size_t default_size;
  client_context_t* next;
};

static bool initialized = false;
/* The registers as they were when we started. */
static vga_registers_t default_registers;
/* The data as it was when we started. */
static bd_t default_data;
/* List of clients. */
static client_context_t* context_list_head = 0;
/* Pointer to the active client. */
static client_context_t* active_context = 0;

/* static int x = 0; */
/* static int y = 0; */
/* static void */
/* print (const char* s) */
/* { */
/*   unsigned short* dest = (unsigned short*) VIDEO_MEMORY_BEGIN; */
/*   for (; *s != 0; ++s) { */
/*     switch (*s) { */
/*     case '\n': */
/*       x = 0; */
/*       ++y; */
/*       break; */
/*     default: */
/*       dest[y * 80 + x] = 0x3400 | *s; */
/*       ++x; */
/*       break; */
/*     } */

/*   } */
/* } */

static void
read_vga_registers (vga_registers_t* r)
{
  r->miscellaneous_output = inb (MISCELLANEOUS_OUTPUT_PORT_READ);
  r->feature_control = inb (FEATURE_CONTROL_PORT_READ);

  outb (SEQUENCER_ADDRESS_PORT, RESET_REGISTER);
  r->reset = inb (SEQUENCER_DATA_PORT);
  outb (SEQUENCER_ADDRESS_PORT, CLOCKING_MODE_REGISTER);
  r->clocking_mode = inb (SEQUENCER_DATA_PORT);
  outb (SEQUENCER_ADDRESS_PORT, MAP_MASK_REGISTER);
  r->map_mask = inb (SEQUENCER_DATA_PORT);
  outb (SEQUENCER_ADDRESS_PORT, CHARACTER_MAP_SELECT_REGISTER);
  r->character_map_select = inb (SEQUENCER_DATA_PORT);
  outb (SEQUENCER_ADDRESS_PORT, MEMORY_MODE_REGISTER);
  r->memory_mode = inb (SEQUENCER_DATA_PORT);

  outb (CRT_ADDRESS_PORT, HORIZONTAL_TOTAL_REGISTER);
  r->horizontal_total = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, HORIZONTAL_DISPLAY_END_REGISTER);
  r->horizontal_display_end = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, START_HORIZONTAL_BLANK_REGISTER);
  r->start_horizontal_blank = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, END_HORIZONTAL_BLANK_REGISTER);
  r->end_horizontal_blank = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, START_HORIZONTAL_RETRACE_REGISTER);
  r->start_horizontal_retrace = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, END_HORIZONTAL_RETRACE_REGISTER);
  r->end_horizontal_retrace = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, VERTICAL_TOTAL_REGISTER);
  r->vertical_total = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, OVERFLOW_REGISTER);
  r->overflow.overflow_byte = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, PRESET_ROW_SCAN_REGISTER);
  r->preset_row_scan = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, MAX_SCAN_LINE_REGISTER);
  r->max_scan_line.max_scan_line_byte = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, CURSOR_START_REGISTER);
  r->cursor_start = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, CURSOR_END_REGISTER);
  r->cursor_end = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, START_ADDRESS_HIGH_REGISTER);
  r->start_address_high = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, START_ADDRESS_LOW_REGISTER);
  r->start_address_low = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_HIGH_REGISTER);
  r->cursor_location_high = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_LOW_REGISTER);
  r->cursor_location_low = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, VERTICAL_RETRACE_START_REGISTER);
  r->vertical_retrace_start = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, VERTICAL_RETRACE_LOW_REGISTER);
  r->vertical_retrace_low = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, VERTICAL_DISPLAY_END_REGISTER);
  r->vertical_display_end = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, OFFSET_REGISTER);
  r->offset = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, UNDERLINE_LOCATION_REGISTER);
  r->underline_location = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, START_VERTICAL_BLANK_REGISTER);
  r->start_vertical_blank = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, END_VERTICAL_BLANK_REGISTER);
  r->end_vertical_blank = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, MODE_CONTROL_REGISTER);
  r->mode_control = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, LINE_COMPARE_REGISTER);
  r->line_compare = inb (CRT_DATA_PORT);

  outb (GRAPHICS_ADDRESS_PORT, SET_RESET_REGISTER);
  r->set_reset = inb (GRAPHICS_DATA_PORT);
  outb (GRAPHICS_ADDRESS_PORT, ENABLE_SET_RESET_REGISTER);
  r->enable_set_reset = inb (GRAPHICS_DATA_PORT);
  outb (GRAPHICS_ADDRESS_PORT, COLOR_COMPARE_REGISTER);
  r->color_compare = inb (GRAPHICS_DATA_PORT);
  outb (GRAPHICS_ADDRESS_PORT, DATA_ROTATE_REGISTER);
  r->data_rotate = inb (GRAPHICS_DATA_PORT);
  outb (GRAPHICS_ADDRESS_PORT, READ_MAP_SELECT_REGISTER);
  r->read_map_select = inb (GRAPHICS_DATA_PORT);
  outb (GRAPHICS_ADDRESS_PORT, MODE_REGISTER);
  r->mode = inb (GRAPHICS_DATA_PORT);
  outb (GRAPHICS_ADDRESS_PORT, MISCELLANEOUS_REGISTER);
  r->miscellaneous = inb (GRAPHICS_DATA_PORT);
  outb (GRAPHICS_ADDRESS_PORT, COLOR_DONT_CARE_REGISTER);
  r->color_dont_care = inb (GRAPHICS_DATA_PORT);
  outb (GRAPHICS_ADDRESS_PORT, BITMASK_REGISTER);
  r->bitmask = inb (GRAPHICS_DATA_PORT);
}

static void
write_vga_registers (const vga_registers_t* r)
{
  outb (MISCELLANEOUS_OUTPUT_PORT_WRITE, r->miscellaneous_output);
  outb (FEATURE_CONTROL_PORT_WRITE, r->feature_control);

  outb (SEQUENCER_ADDRESS_PORT, RESET_REGISTER);
  outb (SEQUENCER_DATA_PORT, r->reset);
  outb (SEQUENCER_ADDRESS_PORT, CLOCKING_MODE_REGISTER);
  outb (SEQUENCER_DATA_PORT, r->clocking_mode);
  outb (SEQUENCER_ADDRESS_PORT, MAP_MASK_REGISTER);
  outb (SEQUENCER_DATA_PORT, r->map_mask);
  outb (SEQUENCER_ADDRESS_PORT, CHARACTER_MAP_SELECT_REGISTER);
  outb (SEQUENCER_DATA_PORT, r->character_map_select);
  outb (SEQUENCER_ADDRESS_PORT, MEMORY_MODE_REGISTER);
  outb (SEQUENCER_DATA_PORT, r->memory_mode);

  outb (CRT_ADDRESS_PORT, HORIZONTAL_TOTAL_REGISTER);
  outb (CRT_DATA_PORT, r->horizontal_total);
  outb (CRT_ADDRESS_PORT, HORIZONTAL_DISPLAY_END_REGISTER);
  outb (CRT_DATA_PORT, r->horizontal_display_end);
  outb (CRT_ADDRESS_PORT, START_HORIZONTAL_BLANK_REGISTER);
  outb (CRT_DATA_PORT, r->start_horizontal_blank);
  outb (CRT_ADDRESS_PORT, END_HORIZONTAL_BLANK_REGISTER);
  outb (CRT_DATA_PORT, r->end_horizontal_blank);
  outb (CRT_ADDRESS_PORT, START_HORIZONTAL_RETRACE_REGISTER);
  outb (CRT_DATA_PORT, r->start_horizontal_retrace);
  outb (CRT_ADDRESS_PORT, END_HORIZONTAL_RETRACE_REGISTER);
  outb (CRT_DATA_PORT, r->end_horizontal_retrace);
  outb (CRT_ADDRESS_PORT, VERTICAL_TOTAL_REGISTER);
  outb (CRT_DATA_PORT, r->vertical_total);
  outb (CRT_ADDRESS_PORT, OVERFLOW_REGISTER);
  outb (CRT_DATA_PORT, r->overflow.overflow_byte);
  outb (CRT_ADDRESS_PORT, PRESET_ROW_SCAN_REGISTER);
  outb (CRT_DATA_PORT, r->preset_row_scan);
  outb (CRT_ADDRESS_PORT, MAX_SCAN_LINE_REGISTER);
  outb (CRT_DATA_PORT, r->max_scan_line.max_scan_line_byte);
  outb (CRT_ADDRESS_PORT, CURSOR_START_REGISTER);
  outb (CRT_DATA_PORT, r->cursor_start);
  outb (CRT_ADDRESS_PORT, CURSOR_END_REGISTER);
  outb (CRT_DATA_PORT, r->cursor_end);
  outb (CRT_ADDRESS_PORT, START_ADDRESS_HIGH_REGISTER);
  outb (CRT_DATA_PORT, r->start_address_high);
  outb (CRT_ADDRESS_PORT, START_ADDRESS_LOW_REGISTER);
  outb (CRT_DATA_PORT, r->start_address_low);
  outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_HIGH_REGISTER);
  outb (CRT_DATA_PORT, r->cursor_location_high);
  outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_LOW_REGISTER);
  outb (CRT_DATA_PORT, r->cursor_location_low);
  outb (CRT_ADDRESS_PORT, VERTICAL_RETRACE_START_REGISTER);
  outb (CRT_DATA_PORT, r->vertical_retrace_start);
  outb (CRT_ADDRESS_PORT, VERTICAL_RETRACE_LOW_REGISTER);
  outb (CRT_DATA_PORT, r->vertical_retrace_low);
  outb (CRT_ADDRESS_PORT, VERTICAL_DISPLAY_END_REGISTER);
  outb (CRT_DATA_PORT, r->vertical_display_end);
  outb (CRT_ADDRESS_PORT, OFFSET_REGISTER);
  outb (CRT_DATA_PORT, r->offset);
  outb (CRT_ADDRESS_PORT, UNDERLINE_LOCATION_REGISTER);
  outb (CRT_DATA_PORT, r->underline_location);
  outb (CRT_ADDRESS_PORT, START_VERTICAL_BLANK_REGISTER);
  outb (CRT_DATA_PORT, r->start_vertical_blank);
  outb (CRT_ADDRESS_PORT, END_VERTICAL_BLANK_REGISTER);
  outb (CRT_DATA_PORT, r->end_vertical_blank);
  outb (CRT_ADDRESS_PORT, MODE_CONTROL_REGISTER);
  outb (CRT_DATA_PORT, r->mode_control);
  outb (CRT_ADDRESS_PORT, LINE_COMPARE_REGISTER);
  outb (CRT_DATA_PORT, r->line_compare);

  outb (GRAPHICS_ADDRESS_PORT, SET_RESET_REGISTER);
  outb (GRAPHICS_DATA_PORT, r->set_reset);
  outb (GRAPHICS_ADDRESS_PORT, ENABLE_SET_RESET_REGISTER);
  outb (GRAPHICS_DATA_PORT, r->enable_set_reset);
  outb (GRAPHICS_ADDRESS_PORT, COLOR_COMPARE_REGISTER);
  outb (GRAPHICS_DATA_PORT, r->color_compare);
  outb (GRAPHICS_ADDRESS_PORT, DATA_ROTATE_REGISTER);
  outb (GRAPHICS_DATA_PORT, r->data_rotate);
  outb (GRAPHICS_ADDRESS_PORT, READ_MAP_SELECT_REGISTER);
  outb (GRAPHICS_DATA_PORT, r->read_map_select);
  outb (GRAPHICS_ADDRESS_PORT, MODE_REGISTER);
  outb (GRAPHICS_DATA_PORT, r->mode);
  outb (GRAPHICS_ADDRESS_PORT, MISCELLANEOUS_REGISTER);
  outb (GRAPHICS_DATA_PORT, r->miscellaneous);
  outb (GRAPHICS_ADDRESS_PORT, COLOR_DONT_CARE_REGISTER);
  outb (GRAPHICS_DATA_PORT, r->color_dont_care);
  outb (GRAPHICS_ADDRESS_PORT, BITMASK_REGISTER);
  outb (GRAPHICS_DATA_PORT, r->bitmask);
}

void
copy_vga_registers (vga_registers_t* dst,
		    const vga_registers_t* src)
{
  memcpy (dst, src, sizeof (vga_registers_t));
}

void
destroy_vga_registers (vga_registers_t* r)
{
  /* Do nothing. */
}

/* static unsigned short */
/* get_start_address (vga_registers_t* r) */
/* { */
/*   return (r->start_address_high << 8) | r->start_address_low; */
/* } */

static void
set_start_address (vga_registers_t* r,
		   unsigned short address)
{
  r->start_address_high = (address >> 8);
  r->start_address_low = address & (0xFF);
  
  if (active_context != 0 && r == &active_context->registers) {
    /* Write through. */
    outb (CRT_ADDRESS_PORT, START_ADDRESS_HIGH_REGISTER);
    outb (CRT_DATA_PORT, r->start_address_high);
    outb (CRT_ADDRESS_PORT, START_ADDRESS_LOW_REGISTER);
    outb (CRT_DATA_PORT, r->start_address_low);
  }
}

static void
set_cursor_location (vga_registers_t* r,
		     unsigned short location)
{
  r->cursor_location_high = (location >> 8);
  r->cursor_location_low = location & (0xFF);
  
  if (active_context != 0 && r == &active_context->registers) {
    /* Write through. */
    outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_HIGH_REGISTER);
    outb (CRT_DATA_PORT, r->cursor_location_high);
    outb (CRT_ADDRESS_PORT, CURSOR_LOCATION_LOW_REGISTER);
    outb (CRT_DATA_PORT, r->cursor_location_low);
  }
}

/* static void */
/* set_line_compare (vga_registers_t* r, */
/* 		  unsigned short line) */
/* { */
/*   r->line_compare = line & (0xFF); */
/*   r->overflow.overflow.line_compare_8 = (line >> 8) & 1; */
/*   r->max_scan_line.max_scan_line.line_compare_9 = (line >> 9) & 1; */
  
/*   if (active_context != 0 && r == &active_context->registers) { */
/*     /\* Write through. *\/ */
/*     outb (CRT_ADDRESS_PORT, LINE_COMPARE_REGISTER); */
/*     outb (CRT_DATA_PORT, r->line_compare); */
/*     outb (CRT_ADDRESS_PORT, OVERFLOW_REGISTER); */
/*     outb (CRT_DATA_PORT, r->overflow.overflow_byte); */
/*     outb (CRT_ADDRESS_PORT, MAX_SCAN_LINE_REGISTER); */
/*     outb (CRT_DATA_PORT, r->max_scan_line.max_scan_line_byte); */
/*   } */
/* } */

static client_context_t*
find_client_context (aid_t aid)
{
  client_context_t* ptr;
  for (ptr = context_list_head; ptr != 0 && ptr->aid != aid; ptr = ptr->next) ;;
  return ptr;
}

static client_context_t*
create_client_context (aid_t aid)
{
  client_context_t* context = malloc (sizeof (client_context_t));
  context->aid = aid;
  copy_vga_registers (&context->registers, &default_registers);
  context->bd = buffer_copy (default_data);
  if (context->bd == -1) {
    syslog ("vga: Could not copy default data buffer");
    exit ();
  }
  context->buffer = buffer_map (context->bd);
  if (context->buffer == 0) {
    syslog ("vga: Could not map client buffer");
    exit ();
  }
  context->default_offset = TEXT_MEMORY_BEGIN - VIDEO_MEMORY_BEGIN;
  context->default_size = TEXT_MEMORY_SIZE;
  context->next = context_list_head;
  context_list_head = context;

  if (subscribe_destroyed (aid, DESTROYED_NO) != 0) {
    syslog ("vga: Could not subscribe to client automaton");
    exit ();
  }

  return context;
}

static void
destroy_client_context (client_context_t* context)
{
  if (context == active_context) {
    active_context = 0;
  }
  destroy_vga_registers (&context->registers);
  buffer_destroy (context->bd);
  free (context);
}

static void
switch_to_context (client_context_t* context)
{
  if (context != active_context) {
    if (active_context != 0) {
      /* Save the video memory. */
      memcpy (active_context->buffer, (const void*)VIDEO_MEMORY_BEGIN, VIDEO_MEMORY_SIZE);
      /* The registers are already saved. */
    }

    /* Restore the registers. */
    write_vga_registers (&context->registers);

    /* Restore the video memory. */
    memcpy ((void*)VIDEO_MEMORY_BEGIN, context->buffer, VIDEO_MEMORY_SIZE);

    active_context = context;
  }
}

static void
assign (client_context_t* context,
	size_t address,
	const void* data,
	size_t size)
{
  if (address < context->default_size &&
      address + size <= context->default_size) {
    void* dst = context->buffer + context->default_offset;
    if (context == active_context) {
      dst = (void*)VIDEO_MEMORY_BEGIN + context->default_offset;
    }
    memcpy (dst + address, data, size);
  }
}

static void
initialize (void)
{
  if (!initialized) {
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
      syslog ("vga: Could not reserve vga ports");
      exit ();
    }

    /* Map in the video memory. */
    if (map ((const void*)VIDEO_MEMORY_BEGIN, (const void*)VIDEO_MEMORY_BEGIN, VIDEO_MEMORY_SIZE) != 0) {
      syslog ("vga: Could not map vga memory");
      exit ();
    }
    /* Clear the text region. */
    memset ((void*)TEXT_MEMORY_BEGIN, 0, TEXT_MEMORY_SIZE);

    /* Read the default registers. */
    read_vga_registers (&default_registers);

    /* Copy the state into a buffer. */
    default_data = buffer_create (size_to_pages (VIDEO_MEMORY_SIZE));
    if (default_data == -1) {
      syslog ("vga: Could not create default buffer");
      exit ();
    }
    void* ptr = buffer_map (default_data);
    if (ptr == 0) {
      syslog ("vga: Could not map default buffer");
      exit ();
    }
    memcpy (ptr, (const void*)VIDEO_MEMORY_BEGIN, VIDEO_MEMORY_SIZE);
    buffer_unmap (default_data);

    initialized = true;
  }
}

static void
end_input_action (bd_t bda,
		  bd_t bdb)
{
  if (bda != -1) {
    buffer_destroy (bda);
  }
  if (bdb != -1) {
    buffer_destroy (bdb);
  }
  finish (NO_ACTION, 0, false, -1, -1);
}

/* typedef struct { */
/*   aid_t aid; */
/* } focus_arg_t; */

/* void */
/* vga_focus (int param, */
/* 	   bd_t bd, */
/* 	   focus_arg_t* a, */
/* 	   size_t buffer_size) */
/* { */
/*   initialize (); */

/*   if (a != 0 && buffer_size == sizeof (focus_arg_t)) { */
/*     client_context_t* context = find_client_context (a->aid); */
/*     if (context != 0) { */
/*       switch_to_context (context); */
/*     } */
/*   } */

/*   finish (NO_ACTION, 0, bd, FINISH_DESTROY); */
/* } */
/* EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, VGA_FOCUS, vga_focus); */

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish (NO_ACTION, 0, false, -1, -1);
}

BEGIN_INPUT (AUTO_PARAMETER, VGA_OP_NO, "vga_op", "vga_op_list", vga_op, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  vga_op_list_t vol;
  size_t count;
  if (vga_op_list_initr (&vol, bda, bdb, &count) != 0) {
    end_input_action (bda, bdb);
  }

  client_context_t* context = find_client_context (aid);
  if (context == 0) {
    context = create_client_context (aid);
  }
  
  /* TODO:  Remove this line. */
  switch_to_context (context);

  for (size_t i = 0; i != count; ++i) {
    vga_op_type_t type;
    if (vga_op_list_next_op_type (&vol, &type) != 0) {
      end_input_action (bda, bdb);
    }

    switch (type) {
    case VGA_SET_START_ADDRESS:
      {
  	size_t address;
  	if (vga_op_list_read_set_start_address (&vol, &address) != 0) {
	  end_input_action (bda, bdb);
  	}
  	set_start_address (&context->registers, address);
      }
      break;
    case VGA_SET_CURSOR_LOCATION:
      {
  	size_t location;
  	if (vga_op_list_read_set_cursor_location (&vol, &location) != 0) {
	  end_input_action (bda, bdb);
  	}
  	set_cursor_location (&context->registers, location);
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
  	assign (context, address, data, size);
      }
      break;
    default:
      end_input_action (bda, bdb);
    }
  }

  end_input_action (bda, bdb);
}

BEGIN_SYSTEM_INPUT (DESTROYED_NO, "", "", destroyed, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  /* Destroy the client context. */
  client_context_t** ptr = &context_list_head;
  for (; *ptr != 0 && (*ptr)->aid != aid; ptr = &(*ptr)->next) ;;
  if (*ptr != 0) {
    client_context_t* temp = *ptr;
    *ptr = temp->next;
    destroy_client_context (temp);
  }

  end_input_action (bda, bdb);
}
