#include <automaton.h>
#include <fifo_scheduler.h>
#include <string.h>
#include <buffer_file.h>

/*
  Driver for a 104-key US keyboard
  ================================
  This driver converts scan codes from a keyboard to ASCII text.
  The ultimate goal should be to support some standard such as UTF-8.

  Design
  ======
  The main data structure is a two dimensional table that translates scan codes to strings.
  The first index into the table is called the modifier and will be explained later.
  The second index into the table is the scan code itself.
  Thus, the basic operation is:

    scan code input -> string[modifier][scan code] -> string output

  The modifier changes the plain of strings implied by a scan code.
  Lets assume the modifier consists of a single bit indicating shift status with the string table defined accordingly.
  When modifier = 0, the scan code for 'a' will be interpretted as 'a.'
  When modifier = 1, the scan code for 'a' will be interpretted as 'A.'
  
  Some of the scan codes, e.g., shift, control, alt, caps lock, etc., change the modifier instead of generating a printable character.
  To accomplish this, we introduce two more tables containing set and toggle bit masks for the modifier.
  When a make code is received, the modifier is updated as:
    modifier = (modifier | set[scan code]) ^ toggle[scan code]
  When a break code is received, the modififer is updated as:
    modififer = modifier & ~set[scan code]

  Let's compare "Caps Lock" to "Ctrl" to see how this works.
  Let's assume that set[Caps Lock] = 0x00, toggle[Caps Lock] = 0x01, set[Ctrl] = 0x10, and toggle[Ctrl] = 0x00.
  Start with an empty modifier:
    modifier = 0
  The "Caps Lock" key is pressed:
    modifier = (modifier | set[Caps Lock]) ^ toggle[Caps Lock]
    modifier = (0 | 0) ^ 0x01
    modifier = 0x01
  The "Caps Lock" key is released:
    modifier = modifier & ~set[Caps Lock]
    modifier = 0x01 & ~0x00
    modifier = 0x01
  The "Ctrl" key is pressed:
    modifier = (modifier | set[Ctrl]) ^ toggle[Ctrl]
    modifier = (0x01 | 0x10) ^ 0x00
    modifier = 0x11
  The "Ctrl" key is released:
    modifier = modifier & ~set[Ctrl]
    modifier = 0x11 & ~0x10
    modifier = 0x01
  The "Caps Lock" key is pressed:
    modifier = (0x01 | set[Caps Lock]) ^ toggle[Caps Lock]
    modifier = (0x01 | 0) ^ 0x01
    modifier = 0x00
  The "Caps Lock" key is released:
    modifier = modifier & ~set[Caps Lock]
    modifier = 0x00 & ~0x00
    modifier = 0x00
*/

/* Key codes. */
#define KEY_ESCAPE 0x01
#define KEY_ONE 0x02
#define KEY_TWO 0x03
#define KEY_THREE 0x04
#define KEY_FOUR 0x05
#define KEY_FIVE 0x06
#define KEY_SIX 0x07
#define KEY_SEVEN 0x08
#define KEY_EIGHT 0x09
#define KEY_NINE 0x0a
#define KEY_ZERO 0x0b
#define KEY_MINUS 0x0c
#define KEY_EQUAL 0x0d
#define KEY_BACKSPACE 0x0e
#define KEY_TAB 0x0f
#define KEY_Q 0x10
#define KEY_W 0x11
#define KEY_E 0x12
#define KEY_R 0x13
#define KEY_T 0x14
#define KEY_Y 0x15
#define KEY_U 0x16
#define KEY_I 0x17
#define KEY_O 0x18
#define KEY_P 0x19
#define KEY_LBRACKET 0x1a
#define KEY_RBRACKET 0x1b
#define KEY_ENTER 0x1c
#define KEY_LCONTROL 0x1d
#define KEY_A 0x1e
#define KEY_S 0x1f
#define KEY_D 0x20
#define KEY_F 0x21
#define KEY_G 0x22
#define KEY_H 0x23
#define KEY_J 0x24
#define KEY_K 0x25
#define KEY_L 0x26
#define KEY_SEMICOLON 0x27
#define KEY_QUOTE 0x28
#define KEY_BACKTICK 0x29
#define KEY_LSHIFT 0x2a
#define KEY_BACKSLASH 0x2b
#define KEY_Z 0x2c
#define KEY_X 0x2d
#define KEY_C 0x2e
#define KEY_V 0x2f
#define KEY_B 0x30
#define KEY_N 0x31
#define KEY_M 0x32
#define KEY_COMMA 0x33
#define KEY_PERIOD 0x34
#define KEY_SLASH 0x35
#define KEY_RSHIFT 0x36
#define KEY_RALT 0x37
#define KEY_LALT 0x38
#define KEY_SPACE 0x39
#define KEY_CAPSLOCK 0x3a
#define KEY_F1 0x3b
#define KEY_F2 0x3c
#define KEY_F3 0x3d
#define KEY_F4 0x3e
#define KEY_F5 0x3f
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44
#define KEY_NUMLOCK 0x45
#define KEY_SCROLLLOCK 0x46
#define KEY_HOME 0x47
#define KEY_UP 0x48
#define KEY_PAGEUP 0x49
#define KEY_RCONTROL 0x4a
#define KEY_LEFT 0x4b
#define KEY_CENTER 0x4c
#define KEY_RIGHT 0x4d
#define KEY_PRINTSCREEN 0x4e
#define KEY_END 0x4f
#define KEY_DOWN 0x50
#define KEY_PAGEDOWN 0x51
#define KEY_INSERT 0x52
#define KEY_DELETE 0x53
#define KEY_SYSRQ 0x54
#define KEY_BREAK 0x55
#define KEY_PAUSE 0x56
#define KEY_F11 0x57
#define KEY_F12 0x58
/* 0x59 */
/* 0x5a */
#define KEY_LWINDOW 0x5b
#define KEY_RWINDOW 0x5c
#define KEY_MENU 0x5d
#define KEY_KP_DECIMAL 0x5e
#define KEY_KP_ENTER 0x5f
#define KEY_KP_ADD 0x60
#define KEY_KP_SUBTRACT 0x61
#define KEY_KP_MULTIPLY 0x62
#define KEY_KP_DIVIDE 0x63
#define KEY_KP_0 0x64
#define KEY_KP_1 0x65
#define KEY_KP_2 0x66
#define KEY_KP_3 0x67
#define KEY_KP_4 0x68
#define KEY_KP_5 0x69
#define KEY_KP_6 0x6a
#define KEY_KP_7 0x6b
#define KEY_KP_8 0x6c
#define KEY_KP_9 0x6d

/* Seven bits are used for a scan code.  The high bit indicates make or break. */
#define BREAK_LIMIT 0x80

/* The scan code for the key bearing the associated symbol. */
#define SCAN_ESCAPE 0x01

#define SCAN_F1 0x3b
#define SCAN_F2 0x3c
#define SCAN_F3 0x3d
#define SCAN_F4 0x3e

#define SCAN_F5 0x3f
#define SCAN_F6 0x40
#define SCAN_F7 0x41
#define SCAN_F8 0x42

#define SCAN_F9 0x43
#define SCAN_F10 0x44
#define SCAN_F11 0x57
#define SCAN_F12 0x58

#define SCAN_BACKTICK 0x29
#define SCAN_ONE 0x02
#define SCAN_TWO 0x03
#define SCAN_THREE 0x04
#define SCAN_FOUR 0x05
#define SCAN_FIVE 0x06
#define SCAN_SIX 0x07
#define SCAN_SEVEN 0x08
#define SCAN_EIGHT 0x09
#define SCAN_NINE 0x0a
#define SCAN_ZERO 0x0b
#define SCAN_MINUS 0x0c
#define SCAN_EQUAL 0x0d
#define SCAN_BACKSPACE 0x0e
#define SCAN_TAB 0x0f
#define SCAN_Q 0x10
#define SCAN_W 0x11
#define SCAN_E 0x12
#define SCAN_R 0x13
#define SCAN_T 0x14
#define SCAN_Y 0x15
#define SCAN_U 0x16
#define SCAN_I 0x17
#define SCAN_O 0x18
#define SCAN_P 0x19
#define SCAN_LBRACKET 0x1a
#define SCAN_RBRACKET 0x1b
#define SCAN_BACKSLASH 0x2b
#define SCAN_CAPSLOCK 0x3a
#define SCAN_A 0x1e
#define SCAN_S 0x1f
#define SCAN_D 0x20
#define SCAN_F 0x21
#define SCAN_G 0x22
#define SCAN_H 0x23
#define SCAN_J 0x24
#define SCAN_K 0x25
#define SCAN_L 0x26
#define SCAN_SEMICOLON 0x27
#define SCAN_QUOTE 0x28
#define SCAN_ENTER 0x1c
#define SCAN_LSHIFT 0x2a
#define SCAN_Z 0x2c
#define SCAN_X 0x2d
#define SCAN_C 0x2e
#define SCAN_V 0x2f
#define SCAN_B 0x30
#define SCAN_N 0x31
#define SCAN_M 0x32
#define SCAN_COMMA 0x33
#define SCAN_PERIOD 0x34
#define SCAN_SLASH 0x35
#define SCAN_RSHIFT 0x36
#define SCAN_LCONTROL 0x1d
#define SCAN_LALT 0x38
#define SCAN_SPACE 0x39

#define SCAN_SYSRQ 0x54
#define SCAN_SCROLLLOCK 0x46

#define SCAN_NUMLOCK 0x45
#define SCAN_KP_MULTIPLY 0x37
#define SCAN_KP_SUBTRACT 0x4a
#define SCAN_KP_ADD 0x4e
#define SCAN_KP_DECIMAL 0x53
#define SCAN_KP_0 0x52
#define SCAN_KP_1 0x4f
#define SCAN_KP_2 0x50
#define SCAN_KP_3 0x51
#define SCAN_KP_4 0x4b
#define SCAN_KP_5 0x4c
#define SCAN_KP_6 0x4d
#define SCAN_KP_7 0x47
#define SCAN_KP_8 0x48
#define SCAN_KP_9 0x49

static unsigned char scan_to_key[BREAK_LIMIT] = {
  [SCAN_ESCAPE] = KEY_ESCAPE,

  [SCAN_F1] = KEY_F1,
  [SCAN_F2] = KEY_F2,
  [SCAN_F3] = KEY_F3,
  [SCAN_F4] = KEY_F4,

  [SCAN_F5] = KEY_F5,
  [SCAN_F6] = KEY_F6,
  [SCAN_F7] = KEY_F7,
  [SCAN_F8] = KEY_F8,

  [SCAN_F9] = KEY_F9,
  [SCAN_F10] = KEY_F10,
  [SCAN_F11] = KEY_F11,
  [SCAN_F12] = KEY_F12,

  [SCAN_BACKTICK] = KEY_BACKTICK,
  [SCAN_ONE] = KEY_ONE,
  [SCAN_TWO] = KEY_TWO,
  [SCAN_THREE] = KEY_THREE,
  [SCAN_FOUR] = KEY_FOUR,
  [SCAN_FIVE] = KEY_FIVE,
  [SCAN_SIX] = KEY_SIX,
  [SCAN_SEVEN] = KEY_SEVEN,
  [SCAN_EIGHT] = KEY_EIGHT,
  [SCAN_NINE] = KEY_NINE,
  [SCAN_ZERO] = KEY_ZERO,
  [SCAN_MINUS] = KEY_MINUS,
  [SCAN_EQUAL] = KEY_EQUAL,
  [SCAN_BACKSPACE] = KEY_BACKSPACE,
  [SCAN_TAB] = KEY_TAB,
  [SCAN_Q] = KEY_Q,
  [SCAN_W] = KEY_W,
  [SCAN_E] = KEY_E,
  [SCAN_R] = KEY_R,
  [SCAN_T] = KEY_T,
  [SCAN_Y] = KEY_Y,
  [SCAN_U] = KEY_U,
  [SCAN_I] = KEY_I,
  [SCAN_O] = KEY_O,
  [SCAN_P] = KEY_P,
  [SCAN_LBRACKET] = KEY_LBRACKET,
  [SCAN_RBRACKET] = KEY_RBRACKET,
  [SCAN_BACKSLASH] = KEY_BACKSLASH,
  [SCAN_CAPSLOCK] = KEY_CAPSLOCK,
  [SCAN_A] = KEY_A,
  [SCAN_S] = KEY_S,
  [SCAN_D] = KEY_D,
  [SCAN_F] = KEY_F,
  [SCAN_G] = KEY_G,
  [SCAN_H] = KEY_H,
  [SCAN_J] = KEY_J,
  [SCAN_K] = KEY_K,
  [SCAN_L] = KEY_L,
  [SCAN_SEMICOLON] = KEY_SEMICOLON,
  [SCAN_QUOTE] = KEY_QUOTE,
  [SCAN_ENTER] = KEY_ENTER,
  [SCAN_LSHIFT] = KEY_LSHIFT,
  [SCAN_Z] = KEY_Z,
  [SCAN_X] = KEY_X,
  [SCAN_C] = KEY_C,
  [SCAN_V] = KEY_V,
  [SCAN_B] = KEY_B,
  [SCAN_N] = KEY_N,
  [SCAN_M] = KEY_M,
  [SCAN_COMMA] = KEY_COMMA,
  [SCAN_PERIOD] = KEY_PERIOD,
  [SCAN_SLASH] = KEY_SLASH,
  [SCAN_RSHIFT] = KEY_RSHIFT,
  [SCAN_LCONTROL] = KEY_LCONTROL,
  [SCAN_LALT] = KEY_LALT,
  [SCAN_SPACE] = KEY_SPACE,

  [SCAN_SYSRQ] = KEY_SYSRQ,
  [SCAN_SCROLLLOCK] = KEY_SCROLLLOCK,

  [SCAN_NUMLOCK] = KEY_NUMLOCK,
  [SCAN_KP_MULTIPLY] = KEY_KP_MULTIPLY,
  [SCAN_KP_SUBTRACT] = KEY_KP_SUBTRACT,
  [SCAN_KP_ADD] = KEY_KP_ADD,
  [SCAN_KP_DECIMAL] = KEY_KP_DECIMAL,

  [SCAN_KP_0] = KEY_KP_0,
  [SCAN_KP_1] = KEY_KP_1,
  [SCAN_KP_2] = KEY_KP_2,
  [SCAN_KP_3] = KEY_KP_3,
  [SCAN_KP_4] = KEY_KP_4,
  [SCAN_KP_5] = KEY_KP_5,
  [SCAN_KP_6] = KEY_KP_6,
  [SCAN_KP_7] = KEY_KP_7,
  [SCAN_KP_8] = KEY_KP_8,
  [SCAN_KP_9] = KEY_KP_9,
};

/* Escaped scan codes. */
#define SCAN_LWINDOW 0x5b
#define SCAN_RALT 0x38
#define SCAN_RWINDOW 0x5c
#define SCAN_MENU 0x5d
#define SCAN_RCONTROL 0x1d

#define SCAN_PRINTSCREEN 0x37
#define SCAN_BREAK 0x46

#define SCAN_INSERT 0x52
#define SCAN_HOME 0x47
#define SCAN_PAGEUP 0x49
#define SCAN_DELETE 0x53
#define SCAN_END 0x4f
#define SCAN_PAGEDOWN 0x51

#define SCAN_UP 0x48
#define SCAN_DOWN 0x50
#define SCAN_LEFT 0x4b
#define SCAN_RIGHT 0x4d

#define SCAN_KP_DIVIDE 0x35
#define SCAN_KP_ENTER 0x1c

static unsigned char escaped_scan_to_key[BREAK_LIMIT] = {
  [SCAN_LWINDOW] = KEY_LWINDOW,
  [SCAN_RALT] = KEY_RALT,
  [SCAN_RWINDOW] = KEY_RWINDOW,
  [SCAN_MENU] = KEY_MENU,
  [SCAN_RCONTROL] = KEY_RCONTROL,

  [SCAN_PRINTSCREEN] = KEY_PRINTSCREEN,
  [SCAN_BREAK] = KEY_BREAK,

  [SCAN_INSERT] = KEY_INSERT,
  [SCAN_HOME] = KEY_HOME,
  [SCAN_PAGEUP] = KEY_PAGEUP,
  [SCAN_DELETE] = KEY_DELETE,
  [SCAN_END] = KEY_END,
  [SCAN_PAGEDOWN] = KEY_PAGEDOWN,

  [SCAN_UP] = KEY_UP,
  [SCAN_DOWN] = KEY_DOWN,
  [SCAN_LEFT] = KEY_LEFT,
  [SCAN_RIGHT] = KEY_RIGHT,

  [SCAN_KP_DIVIDE] = KEY_KP_DIVIDE,
  [SCAN_KP_ENTER] = KEY_KP_ENTER,
};


static bool escaped = false;
static size_t consume = 0;

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
    if (consume != 0) {
      --consume;
      continue;
    }

    unsigned char scan = codes[idx];
    unsigned char key = 0;
    if (scan < BREAK_LIMIT) {
      if (!escaped) {
	key = scan_to_key[scan];
      }
      else {
	escaped = false;
	key = escaped_scan_to_key[scan];
      }

      /* /\* Make. *\/ */
      /* modifier = (modifier | set[c]) ^ toggle[c]; */
      /* if (scan_code_to_string[modifier][c] != 0) { */
      /* 	if (buffer_file_puts (&output_buffer, scan_code_to_string[modifier][c]) == -1) { */
      /* 	  syslog ("kb_us_104: error: Could not write to output buffer"); */
      /* 	  exit (); */
      /* 	} */
      /* } */
    }
    else if (scan == 0xE0) {
      escaped = true;
    }
    else if (scan == 0xE1) {
      key = KEY_PAUSE;
      consume = 5;
    }
    else {
      /* Break. */
      scan -= BREAK_LIMIT;
      buffer_file_puts (&output_buffer, "break\n");
      if (!escaped) {
	key = scan_to_key[scan];
      }
      else {
	escaped = false;
	key = escaped_scan_to_key[scan];
      }
    }

    bfprintf (&output_buffer, "key = %d\n", key);
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
