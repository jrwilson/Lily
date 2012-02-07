#include "kb_us_104.h"
#include <automaton.h>
#include <string.h>
#include <buffer_heap.h>
#include <fifo_scheduler.h>
#include "keyboard.h"

/* Initial capacity of the ASCII array. */
#define INITIAL_CAPACITY 4000

/*
  An array of ASCII characters implemented with a buffer.
  This allows us to output the array when the STRING output fires.
*/
static size_t aarr_size;
static size_t aarr_capacity;
static bd_t aarr_bd;
static kb_us_104_string_t* aarr_struct;
static char* aarr_string;

static bool left_shift = false;
static bool right_shift = false;
static bool caps_lock = false;

#define CAPS_LOCK 0x3A
#define ENTER 0x1C
#define LEFT_SHIFT 0x2A
#define RIGHT_SHIFT 0x36

static char lower_case[128] = {
  /* Escape */

  /* F1-F4 */
  /* F5-F6 */
  /* F7-F8 */
  /* F9-F12 */

  [0x29] = '`',
  [0x02] = '1',
  [0x03] = '2',
  [0x04] = '3',
  [0x05] = '4',
  [0x06] = '5',
  [0x07] = '6',
  [0x08] = '7',
  [0x09] = '8',
  [0x0A] = '9',
  [0x0B] = '0',
  [0x0C] = '-',
  [0x0D] = '=',
  /* Backspace */

  /* Tab */
  [0x10] = 'q',
  [0x11] = 'w',
  [0x12] = 'e',
  [0x13] = 'r',
  [0x14] = 't',
  [0x15] = 'y',
  [0x16] = 'u',
  [0x17] = 'i',
  [0x18] = 'o',
  [0x19] = 'p',
  [0x1A] = '[',
  [0x1B] = ']',
  [0x2B] = '\\',

  /* Caps Lock */
  [0x1E] = 'a',
  [0x1F] = 's',
  [0x20] = 'd',
  [0x21] = 'f',
  [0x22] = 'g',
  [0x23] = 'h',
  [0x24] = 'j',
  [0x25] = 'k',
  [0x26] = 'l',
  [0x27] = ';',
  [0x28] = '\'',
  /* Enter */

  /* Shift */
  [0x2C] = 'z',
  [0x2D] = 'x',
  [0x2E] = 'c',
  [0x2F] = 'v',
  [0x30] = 'b',
  [0x31] = 'n',
  [0x32] = 'm',
  [0x33] = ',',
  [0x34] = '.',
  [0x35] = '/',
  /* Shift */

  /* Control */
  /* Windows */
  /* Alt */
  [0x39] = ' ',
  /* Alt */
  /* Windows */
  /* Menu */
  /* Control */

  /* Print Screen */
  /* Scroll Lock */
  /* Pause */

  /* Insert, Home, Page Up, Delete, End, Page Down */
  /* Up, Down, Left, Right */
  /* Num Lock, / * - + 1234567890. Enter*/
};

static char upper_case[128] = {
  /* Escape */

  /* F1-F4 */
  /* F5-F6 */
  /* F7-F8 */
  /* F9-F12 */

  [0x29] = '~',
  [0x02] = '!',
  [0x03] = '@',
  [0x04] = '#',
  [0x05] = '$',
  [0x06] = '%',
  [0x07] = '^',
  [0x08] = '&',
  [0x09] = '*',
  [0x0A] = '(',
  [0x0B] = ')',
  [0x0C] = '_',
  [0x0D] = '+',
  /* Backspace */

  /* Tab */
  [0x10] = 'Q',
  [0x11] = 'W',
  [0x12] = 'E',
  [0x13] = 'R',
  [0x14] = 'T',
  [0x15] = 'Y',
  [0x16] = 'U',
  [0x17] = 'I',
  [0x18] = 'O',
  [0x19] = 'P',
  [0x1A] = '{',
  [0x1B] = '}',
  [0x2B] = '|',

  /* Caps Lock */
  [0x1E] = 'A',
  [0x1F] = 'S',
  [0x20] = 'D',
  [0x21] = 'F',
  [0x22] = 'G',
  [0x23] = 'H',
  [0x24] = 'J',
  [0x25] = 'K',
  [0x26] = 'L',
  [0x27] = ':',
  [0x28] = '"',
  /* Enter */

  /* Shift */
  [0x2C] = 'Z',
  [0x2D] = 'X',
  [0x2E] = 'C',
  [0x2F] = 'V',
  [0x30] = 'B',
  [0x31] = 'N',
  [0x32] = 'M',
  [0x33] = '<',
  [0x34] = '>',
  [0x35] = '?',
  /* Shift */

  /* Control */
  /* Windows */
  /* Alt */
  [0x39] = ' ',
  /* Alt */
  /* Windows */
  /* Menu */
  /* Control */

  /* Print Screen */
  /* Scroll Lock */
  /* Pause */

  /* Insert, Home, Page Up, Delete, End, Page Down */
  /* Up, Down, Left, Right */
  /* Num Lock, / * - + 1234567890. Enter*/
};

static void
reset_buffer (void)
{
  /* Create an array to store scan codes. */
  aarr_capacity = INITIAL_CAPACITY;
  aarr_size = sizeof (keyboard_scan_code_t) + aarr_capacity;
  aarr_bd = buffer_create (aarr_size);
  void* keycode_ptr = buffer_map (aarr_bd);
  buffer_heap_t keycode_heap;
  buffer_heap_init (&keycode_heap, keycode_ptr, aarr_size);
  aarr_struct = buffer_heap_alloc (&keycode_heap, sizeof (keyboard_scan_code_t));
  aarr_string = buffer_heap_alloc (&keycode_heap, aarr_capacity);
  
  aarr_struct->length = 0;
  aarr_struct->string = (void*)aarr_string - (void*)&aarr_struct->string;
}

static void
put (char c)
{
  /* Resize the output array. */
  if (aarr_struct->length == aarr_capacity) {
    buffer_unmap (aarr_bd);
    buffer_grow (aarr_bd, aarr_capacity);
    aarr_capacity *= 2;
    aarr_size = sizeof (keyboard_scan_code_t) + aarr_capacity;
    void* keycode_ptr = buffer_map (aarr_bd);
    buffer_heap_t keycode_heap;
    buffer_heap_init (&keycode_heap, keycode_ptr, aarr_size);
    aarr_struct = buffer_heap_alloc (&keycode_heap, sizeof (keyboard_scan_code_t));
    aarr_string = buffer_heap_alloc (&keycode_heap, aarr_capacity);
    /* Note:  The count and offset to the array haven't changed. */
  }

  /* Store the scan code. */
  aarr_string[aarr_struct->length++] = c;
}

static void
schedule (void);

void
init (int param,
      bd_t bd,
      void* ptr,
      size_t buffer_size)
{
  reset_buffer ();

  schedule ();
  scheduler_finish (-1, FINISH_NO);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);

void
kb_us_104_scan_code (int param,
		     bd_t bd,
		     void* ptr,
		     size_t size)
{
  if (ptr != 0) {
    buffer_heap_t heap;
    buffer_heap_init (&heap, ptr, size);
    keyboard_scan_code_t* kk = buffer_heap_begin (&heap);
    if (buffer_heap_check (&heap, kk, sizeof (keyboard_scan_code_t))) {
      unsigned char* codes = (void*)&kk->codes + kk->codes;
      if (buffer_heap_check (&heap, codes, kk->count)) {
	for (size_t idx = 0; idx != kk->count; ++idx) {
	  unsigned char c = codes[idx];
	  if (c < 0x80) {
	    /* Make. */
	    switch (c) {
	    case LEFT_SHIFT:
	      left_shift = true;
	      break;
	    case RIGHT_SHIFT:
	      right_shift = true;
	      break;
	    case CAPS_LOCK:
	      caps_lock = !caps_lock;
	      break;
	    default:
	      {
		char t = lower_case[c];
		if (t >= 'a' && t <= 'z') {
		  if (((left_shift || right_shift) && !caps_lock) ||
		      (!(left_shift || right_shift) && caps_lock)) {
		    t = t - 'a' + 'A';
		  }
		}
		else {
		  if (left_shift || right_shift) {
		    t = upper_case[c];
		  }
		}

		if (t != 0) {
		  put (t);
		}
		else {
		  /* TODO */
		  char buf[8];
		  int cnt = snprintf (buf, 8, " %d ", c);
		  for (int i = 0; i != cnt; ++i) {
		    put (buf[i]);
		  }
		}
	      }
	      break;
	    }
	  }
	  else {
	    /* Break. */
	    c -= 0x80;
	    switch (c) {
	    case LEFT_SHIFT:
	      left_shift = false;
	      break;
	    case RIGHT_SHIFT:
	      right_shift = false;
	      break;
	    case CAPS_LOCK:
	      /* Do nothing. */
	      break;
	    }
	  }
	}
      }
    }
  }

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, KB_US_104_SCAN_CODE, kb_us_104_scan_code);

static bool
kb_us_104_printer_op_precondition (void)
{
  /* Don't care if we are bound. */
  return aarr_struct->length != 0;
}

void
kb_us_104_printer_op (int param,
		     size_t bc)
{
  scheduler_remove (KB_US_104_STRING, param);

  if (kb_us_104_printer_op_precondition ()) {
    /* Make a copy for the readers. */
    bd_t bd = aarr_bd;
    /* Reset. */
    reset_buffer ();

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, KB_US_104_STRING, kb_us_104_printer_op);

static void
schedule (void)
{
  if (kb_us_104_printer_op_precondition ()) {
    scheduler_add (KB_US_104_STRING, 0);
  }
}
