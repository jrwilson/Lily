#include <automaton.h>
#include <fifo_scheduler.h>
#include <string.h>
#include <buffer_file.h>
#include <dymem.h>

/*
  Driver for a 104-key US keyboard
  ================================
  This driver converts scan codes from a keyboard to ASCII text.
  The ultimate goal should be to support some standard such as UTF-8.

  The keyboard reports a key stroke via a 7-bit make/break code.
  The make code is emitted when the key is pressed and the break code is emitted when the key is released.
  The break code is the same as the make code except that the high bit is set, i.e., break = make | 0x80.
  The make code 0xe0 indicates an escaped make or break code.
  A key press using the escape sequence will resemble 0xe0 make 0xe0 break.
  The make code 0xe1 indicates the key generates both make and breaks code when pressed and nothing on released.
  The Pause/Break key is an example.
  
  The first step is to convert the scan code into a key code.
  A key code uniquely identifies a key on the keyboard.
  A 104-key keyboard should map scan codes to 104 unique values, one for each key.
*/

/* Mask for break cods. */
#define BREAK_MASK 0x80

/* Normal scan codes . */
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

/* Key codes. */

#define TOTAL_KEYS 256

/* We reserve 0. */
#define KEY_BACKTICK	0x01
#define KEY_ONE		0x02
#define KEY_TWO		0x03
#define KEY_THREE	0x04
#define KEY_FOUR	0x05
#define KEY_FIVE	0x06
#define KEY_SIX		0x07
#define KEY_SEVEN	0x08
#define KEY_EIGHT	0x09
#define KEY_NINE	0x0a
#define KEY_ZERO	0x0b
#define KEY_MINUS	0x0c
#define KEY_EQUAL	0x0d
#define KEY_BACKSPACE	0x0e

#define KEY_TAB		0x0f
#define KEY_Q		0x10
#define KEY_W		0x11
#define KEY_E		0x12
#define KEY_R		0x13
#define KEY_T		0x14
#define KEY_Y		0x15
#define KEY_U		0x16
#define KEY_I		0x17
#define KEY_O		0x18
#define KEY_P		0x19
#define KEY_LBRACKET	0x1a
#define KEY_RBRACKET	0x1b
#define KEY_BACKSLASH	0x1c

#define KEY_CAPSLOCK	0x1d
#define KEY_A		0x1e
#define KEY_S		0x1f
#define KEY_D		0x20
#define KEY_F		0x21
#define KEY_G		0x22
#define KEY_H		0x23
#define KEY_J		0x24
#define KEY_K		0x25
#define KEY_L		0x26
#define KEY_SEMICOLON	0x27
#define KEY_QUOTE	0x28
#define KEY_ENTER	0x29

#define KEY_LSHIFT	0x2a
#define KEY_Z		0x2b
#define KEY_X		0x2c
#define KEY_C		0x2d
#define KEY_V		0x2e
#define KEY_B		0x2f
#define KEY_N		0x30
#define KEY_M		0x31
#define KEY_COMMA	0x32
#define KEY_PERIOD	0x33
#define KEY_SLASH	0x34
#define KEY_RSHIFT	0x35

#define KEY_LCTRL	0x36
#define KEY_LWIN	0x37
#define KEY_LALT	0x38
#define KEY_SPACE	0x39
#define KEY_RALT	0x3a
#define KEY_RWIN	0x3b
#define KEY_MENU	0x3c
#define KEY_RCTRL	0x3d

#define KEY_NUMLOCK	0x3e
#define KEY_KP_DIVIDE	0x3f
#define KEY_KP_MULTIPLY	0x40
#define KEY_KP_SUBTRACT	0x41
#define KEY_KP_7	0x42
#define KEY_KP_8	0x43
#define KEY_KP_9	0x44
#define KEY_KP_ADD	0x45
#define KEY_KP_4	0x46
#define KEY_KP_5	0x47
#define KEY_KP_6	0x48
#define KEY_KP_1	0x49
#define KEY_KP_2	0x4a
#define KEY_KP_3	0x4b
#define KEY_KP_ENTER	0x4c
#define KEY_KP_0	0x4d
#define KEY_KP_DECIMAL	0x4e

#define KEY_INSERT	0x4f
#define KEY_HOME	0x50
#define KEY_PAGEUP	0x51
#define KEY_DELETE	0x52
#define KEY_END		0x53
#define KEY_PAGEDOWN	0x54

#define KEY_UP		0x55
#define KEY_LEFT	0x56
#define KEY_DOWN	0x57
#define KEY_RIGHT	0x58

#define KEY_F1		0x59
#define KEY_F2		0x5a
#define KEY_F3		0x5b
#define KEY_F4		0x5c

#define KEY_F5		0x5d
#define KEY_F6		0x5e
#define KEY_F7		0x5f
#define KEY_F8		0x60

#define KEY_F9		0x61
#define KEY_F10		0x62
#define KEY_F11		0x63
#define KEY_F12		0x64

#define KEY_PRTSCRN	0x65
#define KEY_SCROLLLOCK	0x66
#define KEY_PAUSE	0x67

#define KEY_ESCAPE	0x68

/* This table translates scan codes to key codes. */
static unsigned char scan_to_key[BREAK_MASK] = {
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
  [SCAN_LCONTROL] = KEY_LCTRL,
  [SCAN_LALT] = KEY_LALT,
  [SCAN_SPACE] = KEY_SPACE,

  [SCAN_SYSRQ] = KEY_PRTSCRN,
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

/* This code translates escaped scan codes to key codes. */
static unsigned char escaped_scan_to_key[BREAK_MASK] = {
  [SCAN_LWINDOW] = KEY_LWIN,
  [SCAN_RALT] = KEY_RALT,
  [SCAN_RWINDOW] = KEY_RWIN,
  [SCAN_MENU] = KEY_MENU,
  [SCAN_RCONTROL] = KEY_RCTRL,

  [SCAN_PRINTSCREEN] = KEY_PRTSCRN,
  [SCAN_BREAK] = KEY_PAUSE,

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

/* Variables for parsing scan codes. */
static bool escaped = false;
static size_t consume = 0;

/* Modifiers and their weight. */
static int shift = 0;
static int alt = 0;
static int ctrl = 0;
static int lshift = 0;
static int rshift = 0;
static int lalt = 0;
static int ralt = 0;
static int lctrl = 0;
static int rctrl = 0;

#define SHIFT_WEIGHT (1 << 0)
#define ALT_WEIGHT (1 << 1)
#define CTRL_WEIGHT (1 << 2)
#define LSHIFT_WEIGHT (1 << 3)
#define RSHIFT_WEIGHT (1 << 4)
#define LALT_WEIGHT (1 << 5)
#define RALT_WEIGHT (1 << 6)
#define LCTRL_WEIGHT (1 << 7)
#define RCTRL_WEIGHT (1 << 8)

/* Additional state variables. */
static int caps = 0;
static int scroll = 0;
static int num = 0;

/* The total number of modifier states is 2^9 = 512. */
#define MODIFIER_STATES 512

typedef enum {
  ACTION_NONE = 0,

  ACTION_a = 'a',
  ACTION_b = 'b',
  ACTION_c = 'c',
  ACTION_d = 'd',
  ACTION_e = 'e',
  ACTION_f = 'f',
  ACTION_g = 'g',
  ACTION_h = 'h',
  ACTION_i = 'i',
  ACTION_j = 'j',
  ACTION_k = 'k',
  ACTION_l = 'l',
  ACTION_m = 'm',
  ACTION_n = 'n',
  ACTION_o = 'o',
  ACTION_p = 'p',
  ACTION_q = 'q',
  ACTION_r = 'r',
  ACTION_s = 's',
  ACTION_t = 't',
  ACTION_u = 'u',
  ACTION_v = 'v',
  ACTION_w = 'w',
  ACTION_x = 'x',
  ACTION_y = 'y',
  ACTION_z = 'z',

  ACTION_A = 'A',
  ACTION_B = 'B',
  ACTION_C = 'C',
  ACTION_D = 'D',
  ACTION_E = 'E',
  ACTION_F = 'F',
  ACTION_G = 'G',
  ACTION_H = 'H',
  ACTION_I = 'I',
  ACTION_J = 'J',
  ACTION_K = 'K',
  ACTION_L = 'L',
  ACTION_M = 'M',
  ACTION_N = 'N',
  ACTION_O = 'O',
  ACTION_P = 'P',
  ACTION_Q = 'Q',
  ACTION_R = 'R',
  ACTION_S = 'S',
  ACTION_T = 'T',
  ACTION_U = 'U',
  ACTION_V = 'V',
  ACTION_W = 'W',
  ACTION_X = 'X',
  ACTION_Y = 'Y',
  ACTION_Z = 'Z',

  ACTION_SHIFT,
  ACTION_ALT,
  ACTION_CTRL,
} action_t;

/* Lookup table that maps a modifier state to an array of actions. */
static action_t* modifier_state_to_actions[MODIFIER_STATES];

/* Insert a modifier group. */
static void
insert_modifier_group (unsigned int modifier)
{
  if (modifier_state_to_actions[modifier] != 0) {
    syslog ("kb_us_104: error: modifier group exists");
    exit ();
  }
  modifier_state_to_actions[modifier] = malloc (TOTAL_KEYS * sizeof (action_t));
  memset (modifier_state_to_actions[modifier], 0, TOTAL_KEYS * sizeof (action_t));
}

/* Insert an action into the table. */
static void
insert_action (unsigned int modifier,
	       unsigned char key,
	       action_t action)
{
  if (modifier_state_to_actions[modifier] == 0) {
    syslog ("kb_us_104: error: modifier group does not exist");
    exit ();
  }
  modifier_state_to_actions[modifier][key] = action;
}

/* Insert an action into all groups. */
static void
insert_action_all (unsigned char key,
		   action_t action)
{
  for (unsigned int modifier = 0; modifier != MODIFIER_STATES; ++modifier) {
    if (modifier_state_to_actions[modifier] != 0) {
      modifier_state_to_actions[modifier][key] = action;
    }
  }
}

static action_t
lookup_action (unsigned char key)
{
  unsigned int modifier = 0;
  if (shift) {
    modifier |= SHIFT_WEIGHT;
  }
  if (alt) {
    modifier |= ALT_WEIGHT;
  }
  if (ctrl) {
    modifier |= CTRL_WEIGHT;
  }
  if (lalt) {
    modifier |= LALT_WEIGHT;
  }
  if (ralt) {
    modifier |= RALT_WEIGHT;
  }
  if (lctrl) {
    modifier |= LCTRL_WEIGHT;
  }
  if (rctrl) {
    modifier |= RCTRL_WEIGHT;
  }
  if (lshift) {
    modifier |= LSHIFT_WEIGHT;
  }
  if (rshift) {
    modifier |= RSHIFT_WEIGHT;
  }

  if (modifier_state_to_actions[modifier] != 0) {
    return modifier_state_to_actions[modifier][key];
  }
  else {
    return ACTION_NONE;
  }
}
	       

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

    /* Set up the modifier groups. */
    insert_modifier_group (          0 |          0 |            0);
    insert_modifier_group (          0 |          0 | SHIFT_WEIGHT);
    insert_modifier_group (          0 | ALT_WEIGHT |            0);
    insert_modifier_group (          0 | ALT_WEIGHT | SHIFT_WEIGHT);
    insert_modifier_group (CTRL_WEIGHT |          0 |            0);
    insert_modifier_group (CTRL_WEIGHT |          0 | SHIFT_WEIGHT);
    insert_modifier_group (CTRL_WEIGHT | ALT_WEIGHT |            0);
    insert_modifier_group (CTRL_WEIGHT | ALT_WEIGHT | SHIFT_WEIGHT);

    /* Set up the modifier keys. */
    insert_action_all (KEY_LSHIFT, ACTION_SHIFT);
    insert_action_all (KEY_RSHIFT, ACTION_SHIFT);
    insert_action_all (KEY_LALT, ACTION_ALT);
    insert_action_all (KEY_RALT, ACTION_ALT);
    insert_action_all (KEY_LCTRL, ACTION_CTRL);
    insert_action_all (KEY_RCTRL, ACTION_CTRL);

    insert_action (0, KEY_A, ACTION_a);
    insert_action (0, KEY_B, ACTION_b);
    insert_action (0, KEY_C, ACTION_c);
    insert_action (0, KEY_D, ACTION_d);
    insert_action (0, KEY_E, ACTION_e);
    insert_action (0, KEY_F, ACTION_f);
    insert_action (0, KEY_G, ACTION_g);
    insert_action (0, KEY_H, ACTION_h);
    insert_action (0, KEY_I, ACTION_i);
    insert_action (0, KEY_J, ACTION_j);
    insert_action (0, KEY_K, ACTION_k);
    insert_action (0, KEY_L, ACTION_l);
    insert_action (0, KEY_M, ACTION_m);
    insert_action (0, KEY_N, ACTION_n);
    insert_action (0, KEY_O, ACTION_o);
    insert_action (0, KEY_P, ACTION_p);
    insert_action (0, KEY_Q, ACTION_q);
    insert_action (0, KEY_R, ACTION_r);
    insert_action (0, KEY_S, ACTION_s);
    insert_action (0, KEY_T, ACTION_t);
    insert_action (0, KEY_U, ACTION_u);
    insert_action (0, KEY_V, ACTION_v);
    insert_action (0, KEY_W, ACTION_w);
    insert_action (0, KEY_X, ACTION_x);
    insert_action (0, KEY_Y, ACTION_y);
    insert_action (0, KEY_Z, ACTION_z);

    insert_action (SHIFT_WEIGHT, KEY_A, ACTION_A);
    insert_action (SHIFT_WEIGHT, KEY_B, ACTION_B);
    insert_action (SHIFT_WEIGHT, KEY_C, ACTION_C);
    insert_action (SHIFT_WEIGHT, KEY_D, ACTION_D);
    insert_action (SHIFT_WEIGHT, KEY_E, ACTION_E);
    insert_action (SHIFT_WEIGHT, KEY_F, ACTION_F);
    insert_action (SHIFT_WEIGHT, KEY_G, ACTION_G);
    insert_action (SHIFT_WEIGHT, KEY_H, ACTION_H);
    insert_action (SHIFT_WEIGHT, KEY_I, ACTION_I);
    insert_action (SHIFT_WEIGHT, KEY_J, ACTION_J);
    insert_action (SHIFT_WEIGHT, KEY_K, ACTION_K);
    insert_action (SHIFT_WEIGHT, KEY_L, ACTION_L);
    insert_action (SHIFT_WEIGHT, KEY_M, ACTION_M);
    insert_action (SHIFT_WEIGHT, KEY_N, ACTION_N);
    insert_action (SHIFT_WEIGHT, KEY_O, ACTION_O);
    insert_action (SHIFT_WEIGHT, KEY_P, ACTION_P);
    insert_action (SHIFT_WEIGHT, KEY_Q, ACTION_Q);
    insert_action (SHIFT_WEIGHT, KEY_R, ACTION_R);
    insert_action (SHIFT_WEIGHT, KEY_S, ACTION_S);
    insert_action (SHIFT_WEIGHT, KEY_T, ACTION_T);
    insert_action (SHIFT_WEIGHT, KEY_U, ACTION_U);
    insert_action (SHIFT_WEIGHT, KEY_V, ACTION_V);
    insert_action (SHIFT_WEIGHT, KEY_W, ACTION_W);
    insert_action (SHIFT_WEIGHT, KEY_X, ACTION_X);
    insert_action (SHIFT_WEIGHT, KEY_Y, ACTION_Y);
    insert_action (SHIFT_WEIGHT, KEY_Z, ACTION_Z);
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
    bool make = false;
    if (scan < BREAK_MASK) {
      make = true;
      if (!escaped) {
	key = scan_to_key[scan];
      }
      else {
	escaped = false;
	key = escaped_scan_to_key[scan];
      }
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
      make = false;
      scan -= BREAK_MASK;
      if (!escaped) {
	key = scan_to_key[scan];
      }
      else {
	escaped = false;
	key = escaped_scan_to_key[scan];
      }
    }

    action_t action = lookup_action (key);

    if (make) {
      if ((action >= 'a' && action <= 'z') ||
	  (action >= 'A' && action <= 'Z')) {
	buffer_file_put (&output_buffer, action);
      }
      else {
	switch (action) {
	case ACTION_SHIFT:
	  ++shift;
	  break;
	case ACTION_ALT:
	  ++alt;
	  break;
	case ACTION_CTRL:
	  ++ctrl;
	  break;
	default:
	  /* Do nothing. */
	  break;
	}
      }
    }
    else {
      switch (action) {
      case ACTION_SHIFT:
	--shift;
	break;
      case ACTION_ALT:
	--alt;
	break;
      case ACTION_CTRL:
	--ctrl;
	break;
      default:
	/* Do nothing. */
	break;
      }
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
