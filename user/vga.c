#include "vga.h"
#include <automaton.h>
#include <io.h>
#include <dymem.h>
#include <string.h>

/* TODO:  We don't mess with fonts or attributes.  We also assume a 16-color 80x25 display starting at 0xB8000. */

#define VIDEO_MEMORY_BEGIN 0xB8000
#define VIDEO_MEMORY_END   0xC0000
#define VIDEO_MEMORY_SIZE (VIDEO_MEMORY_END - VIDEO_MEMORY_BEGIN)

/* General or external registers. */
#define MISCELLANEOUS_OUTPUT_PORT_READ 0x3CC
#define MISCELLANEOUS_OUTPUT_PORT_WRITE 0x3C2
#define FEATURE_CONTROL_PORT_READ 0x3CA
/* Assuming color. */
#define FEATURE_CONTROL_PORT_WRITE 0x3DA
#define INPUT_STATUS0_PORT_READ 0x3C2
/* Assuming color. */
#define INPUT_STATUS1_PORT_READ 0x3DA

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
  unsigned char input_status0;
  unsigned char input_status1;

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
  unsigned char overflow;
  unsigned char preset_row_scan;
  unsigned char max_scan_line;
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
  client_context_t* next;
};

static bool initialized = false;
/* The registers as they were when we started. */
static vga_registers_t default_registers;
/* List of clients. */
static client_context_t* context_list_head = 0;
/* Pointer to the active client. */
static client_context_t* active_context = 0;

static int x = 0;
static int y = 0;

static void
print (const char* s)
{
  unsigned short* dest = (unsigned short*) VIDEO_MEMORY_BEGIN;
  for (; *s != 0; ++s) {
    switch (*s) {
    case '\n':
      x = 0;
      ++y;
      break;
    default:
      dest[y * 80 + x] = 0x3400 | *s;
      ++x;
      break;
    }

  }
}

static void
read_vga_registers (vga_registers_t* r)
{
  r->miscellaneous_output = inb (MISCELLANEOUS_OUTPUT_PORT_READ);
  r->feature_control = inb (FEATURE_CONTROL_PORT_READ);
  r->input_status0 = inb (INPUT_STATUS0_PORT_READ);
  r->input_status1 = inb (INPUT_STATUS1_PORT_READ);

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
  r->overflow = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, PRESET_ROW_SCAN_REGISTER);
  r->preset_row_scan = inb (CRT_DATA_PORT);
  outb (CRT_ADDRESS_PORT, MAX_SCAN_LINE_REGISTER);
  r->max_scan_line = inb (CRT_DATA_PORT);
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

static unsigned short
get_start_address (vga_registers_t* r)
{
  return (r->start_address_high << 8) | r->start_address_low;
}

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
  context->bd = buffer_create (VIDEO_MEMORY_SIZE);
  context->buffer = buffer_map (context->bd);
  context->next = context_list_head;
  context_list_head = context;

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
      /* Save the existing context. */
      memcpy (active_context->buffer, (const void*)VIDEO_MEMORY_BEGIN, VIDEO_MEMORY_SIZE);
    }

    /* Restore the given context. */
    memcpy ((void*)VIDEO_MEMORY_BEGIN, context->buffer, VIDEO_MEMORY_SIZE);

    active_context = context;
  }
}

static void
assign (client_context_t* context,
	size_t offset,
	const unsigned short* data,
	size_t count)
{
  unsigned short* dest = context->buffer;
  if (context == active_context) {
    dest = (unsigned short*)VIDEO_MEMORY_BEGIN;
  }

  unsigned short* begin = &dest[offset];
  unsigned short* end = &dest[offset + count];

  if (end <= (unsigned short*)VIDEO_MEMORY_END) {
    memcpy (begin, data, count * sizeof (unsigned short));
  }
  else {
    /* Wrap around. */
    size_t diff = offset + count - (VIDEO_MEMORY_SIZE / 2);
    memcpy (begin, data, (count - diff) * sizeof (unsigned short));
    memcpy (dest, &data[count - diff], diff * sizeof (unsigned short));
  }
}

static void
initialize (void)
{
  if (!initialized) {
    /* Reserve all of the VGA ports.*/
    reserve_port (MISCELLANEOUS_OUTPUT_PORT_READ);
    reserve_port (MISCELLANEOUS_OUTPUT_PORT_WRITE);
    reserve_port (FEATURE_CONTROL_PORT_READ);
    reserve_port (FEATURE_CONTROL_PORT_WRITE);
    reserve_port (INPUT_STATUS0_PORT_READ);
    reserve_port (INPUT_STATUS1_PORT_READ);

    reserve_port (SEQUENCER_ADDRESS_PORT);
    reserve_port (SEQUENCER_DATA_PORT);

    reserve_port (CRT_ADDRESS_PORT);
    reserve_port (CRT_DATA_PORT);

    reserve_port (GRAPHICS_ADDRESS_PORT);
    reserve_port (GRAPHICS_DATA_PORT);

    /* Map in the video memory. */
    map ((const void*)VIDEO_MEMORY_BEGIN, (const void*)VIDEO_MEMORY_BEGIN, VIDEO_MEMORY_SIZE);

    /* Read the default registers. */
    read_vga_registers (&default_registers);

    initialized = true;
  }
}

void
init (void)
{
  initialize ();

  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, LILY_ACTION_INIT, init);

typedef struct {
  aid_t aid;
} focus_arg_t;

void
focus (const void* param,
       const focus_arg_t* a,
       size_t a_size)
{
  initialize ();

  if (a != 0 && a_size == sizeof (focus_arg_t)) {
    client_context_t* context = find_client_context (a->aid);
    if (context != 0) {
      switch_to_context (context);
    }
  }

  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, VGA_FOCUS, focus);

void
op_sense (aid_t aid)
{
  initialize ();

  size_t count = binding_count (VGA_OP_SENSE, (const void*)aid);
  if (count != 0) {
    /* Create client context if necessary. */
    if (find_client_context (aid) == 0) {
      create_client_context (aid);
    }
  }
  else {
    /* Destroy client context if necessary. */
    client_context_t** ptr = &context_list_head;
    for (; *ptr != 0 && (*ptr)->aid != aid; ptr = &(*ptr)->next) ;;
    if (*ptr != 0) {
      client_context_t* temp = *ptr;
      *ptr = temp->next;
      destroy_client_context (temp);
    }
  }

  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, AUTO_PARAMETER, VGA_OP_SENSE, op_sense);

void
op (aid_t aid,
    const op_t* a,
    size_t a_size)
{
  initialize ();

  if (binding_count (VGA_OP_SENSE, (const void*)aid) != 0 && a != 0 && a_size == sizeof (op_t)) {
    client_context_t* context = find_client_context (aid);
    if (context == 0) {
      context = create_client_context (aid);
    }

    /* TODO:  Remove this line. */
    switch_to_context (context);

    switch (a->type) {
    case ASSIGN_VALUE:
      if (a->arg.assign_value.count <= 128) {
  	assign (context, a->arg.assign_value.offset, a->arg.assign_value.data, a->arg.assign_value.count);
      }
      break;
    case ASSIGN_BUFFER:
      // TODO
      break;
    case COPY:
      // TODO
      break;
    }
  }
  
  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, VGA_OP, op);
