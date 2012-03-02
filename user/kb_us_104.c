#include <automaton.h>
#include <fifo_scheduler.h>
#include <string.h>
#include <buffer_file.h>

/*
  Driver for a 104-key US keyboard
  ================================
  This driver converts scan codes from a keyboard to ASCII text.
  The ultimate goal should be to support some standard such as UTF-8.
*/

static bool left_shift = false;
static bool right_shift = false;
static bool caps_lock = false;
static bool control = false;
static bool alt = false;

#define BREAK 0x80

#define CAPS_LOCK 0x3A
#define ENTER 0x1C
#define LEFT_SHIFT 0x2A
#define RIGHT_SHIFT 0x36
#define CONTROL 0x1D
#define ALT 0x38

static char lower_case[128] = {
  [0x01] = 27, /* Escape */

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
  [0x0E] = '\b', /* Backsapce */

  [0x0F] = '\t', /* Tab */
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
  [0x1C] = '\n', /* Enter */

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
  /* Space */
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

#define SCAN_CODE_NO 1
#define TEXT_NO 2

/* Initialization flag. */
static bool initialized = false;

/* Processed scan codes. */
static bool output_buffer_initialized = false;
static bd_t output_buffer_bd = -1;
static buffer_file_t output_buffer;

static void
initialize_output_buffer (void)
{
  if (!output_buffer_initialized) {
    output_buffer_initialized = true;
    if (buffer_file_initc (&output_buffer, output_buffer_bd) == -1) {
      syslog ("kb_us_104: error: Could not initialize output buffer");
      exit ();
    }
  }
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    /* Allocate the output buffer. */
    output_buffer_bd = buffer_create (1);
    if (output_buffer_bd == -1) {
      syslog ("kb_us_104: error: Could not create output buffer");
      exit ();
    }
  }
}

static void
schedule (void);

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
  schedule ();
  scheduler_finish (false, -1, -1);
}

static void
end_output_action (bool output_fired,
		   bd_t bda,
		   bd_t bdb)
{
  schedule ();
  scheduler_finish (output_fired, bda, bdb);
}

BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  end_input_action (bda, bdb);
}

/* scan_code
   ---------
   Convert scancodes into ASCII text.
   
   Post: if successful, then output_buffer_initialized == true && the output buffer is not empty
 */
BEGIN_INPUT (NO_PARAMETER, SCAN_CODE_NO, "scan_code", "buffer_file", scan_code, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) == -1) {
    end_input_action (bda, bdb);
  }

  size_t size = buffer_file_size (&input_buffer);
  const unsigned char* codes = buffer_file_readp (&input_buffer, size);
  if (codes == 0) {
    end_input_action (bda, bdb);
  }

  initialize_output_buffer ();

  for (size_t idx = 0; idx != size; ++idx) {
    unsigned char c = codes[idx];
    if (c < BREAK) {
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
      case CONTROL:
	control = true;
	break;
      case ALT:
	alt = true;
	break;
      default:
	{
	  char t = lower_case[c];
	  /*
	    The first clause says that an alphabetic character should be capitalized if either shifting or caps lock exclusively.
	    The second clause says that all other characters can be shifted if a definition exists.
	  */
	  if ((t >= 'a' && t <= 'z') &&
	      (((left_shift || right_shift) && !caps_lock) ||
	       (!(left_shift || right_shift) && caps_lock))) {
	    t = upper_case[c];
	  }
	  else if ((left_shift || right_shift) && upper_case[c] != 0) {
	    t = upper_case[c];
	  }
	  
	  if (t != 0) {
	    if (buffer_file_put (&output_buffer, t) == -1) {
	      if (output_buffer_bd == -1) {
		syslog ("kb_us_104: error: Could not write to output buffer");
		exit ();
	      }
	    }
	  }
	  /* else { */
	  /*   /\* TODO *\/ */
	  /*   char buf[256]; */
	  /*   int cnt = snprintf (buf, 256, " %x ", c); */
	  /*   for (int i = 0; i != cnt; ++i) { */
	  /*     put (buf[i]); */
	  /*   } */
	  /* } */
	}
	break;
      }
    }
    else {
      /* Break. */
      c -= BREAK;
      switch (c) {
      case LEFT_SHIFT:
	left_shift = false;
	break;
      case RIGHT_SHIFT:
	right_shift = false;
	break;
      case CONTROL:
	control = false;
	break;
      case ALT:
	alt = false;
	break;
      }
    }
  }

  end_input_action (bda, bdb);
}

/* text
   ----
   Output a string of ASCII text.
   
   Pre:  output_buffer_initialized == true && the output buffer is not empty
   Post: output_buffer_initialized == false
 */
static bool
text_precondition (void)
{
  return output_buffer_initialized && buffer_file_size (&output_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, TEXT_NO, "text", "buffer_file", text, int param)
{
  initialize ();
  scheduler_remove (TEXT_NO, param);

  if (text_precondition ()) {
    output_buffer_initialized = false;
    end_output_action (true, output_buffer_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

static void
schedule (void)
{
  if (text_precondition ()) {
    scheduler_add (TEXT_NO, 0);
  }
}
