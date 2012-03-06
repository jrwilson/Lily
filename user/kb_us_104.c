#include <automaton.h>
#include <fifo_scheduler.h>
#include <string.h>
#include <buffer_file.h>
#include <dymem.h>

/*
  Driver for a 104-key US keyboard
  ================================
  This driver converts scan codes from a keyboard to ASCII text.
  The ultimate goal should be to support different mappings and character sets like the Linux keyboard driver.

  Design
  ======
  The design was inspired by the Linux keyboard driver.

  The driver performs a number of translations according to the following diagram:

    (scan code) -> (key code, make/break) -> (symbol, make/break) -> (interpretation, make/break)

  Keyboards report a key stroke via a 7-bit make/break code called a scan code.
  The make code is emitted when the key is pressed and the break code is emitted when the key is released.
  The break code is the same as the make code except that the high bit is set, i.e., break = make | 0x80.
  The make code 0xe0 indicates an escaped make or break code.
  A key press using the escape sequence will resemble 0xe0 make 0xe0 break.
  The make code 0xe1 indicates the key generates both make and breaks code when pressed and nothing on released.
  The Pause/Break key is an example.
  
  A key code represents a key on a virtual keyboard, i.e., a location and symbol.
  The translation from scan code to key code is necessary to smooth out variations among keyboards.
  A keyboard with 104 physical keys should imply the use of 104 key codes; no more; no less.

  Symbols are a pure abstraction that represent actions to be performed by the keyboard driver.
  Actions are associated with a symbol through a character set.
  Actions typically either write bytes to the output stream, e.g., print an ASCII "A", or change the internal state of the keyboard driver, e.g., set the Caps Lock flag.
  The translation from key code to symbol uses a set of internal flags called modifiers.
  The modifiers are: Shift, Alt, Ctrl, LShift, RShift, LAlt, RAlt, LCtrl, and RCtrl.
  Each modifier has a weight corresponding to a power of 2.
  When translating a key code, the weight of each modifier is summed to produce an index into a two-dimensional array that translates (modifier, key code) into a symbol.
  
  TODO:  Talk about Caps Lock and Num Lock.

  The interpretation stage uses a table of function pointers to turn symbols into actions.
  A function pointer is invoked for both make and break with a flag to discriminate.
  The default character set is ASCII.
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
#define KEY_1		0x02
#define KEY_2		0x03
#define KEY_3		0x04
#define KEY_4		0x05
#define KEY_5		0x06
#define KEY_6		0x07
#define KEY_7		0x08
#define KEY_8		0x09
#define KEY_9		0x0a
#define KEY_0		0x0b
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
  [SCAN_ONE] = KEY_1,
  [SCAN_TWO] = KEY_2,
  [SCAN_THREE] = KEY_3,
  [SCAN_FOUR] = KEY_4,
  [SCAN_FIVE] = KEY_5,
  [SCAN_SIX] = KEY_6,
  [SCAN_SEVEN] = KEY_7,
  [SCAN_EIGHT] = KEY_8,
  [SCAN_NINE] = KEY_9,
  [SCAN_ZERO] = KEY_0,
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
static bool caps = false;
static bool num = false;

/* The total number of modifier states is 2^9 = 512. */
#define MODIFIER_STATES 512

/* The symbols are set up so ASCII characters have the same value.  This allows them to be handled efficiently. */
typedef enum {
  SYMBOL_ASCII_NULL,
  SYMBOL_ASCII_SOH,
  SYMBOL_ASCII_STX,
  SYMBOL_ASCII_ETX,
  SYMBOL_ASCII_EOT,
  SYMBOL_ASCII_ENQ,
  SYMBOL_ASCII_ACK,
  SYMBOL_ASCII_BEL,
  SYMBOL_ASCII_BS,
  SYMBOL_ASCII_HT,
  SYMBOL_ASCII_LF,
  SYMBOL_ASCII_VT,
  SYMBOL_ASCII_FF,
  SYMBOL_ASCII_CR,
  SYMBOL_ASCII_SO,
  SYMBOL_ASCII_SI,
  SYMBOL_ASCII_DLE,
  SYMBOL_ASCII_DC1,
  SYMBOL_ASCII_DC2,
  SYMBOL_ASCII_DC3,
  SYMBOL_ASCII_DC4,
  SYMBOL_ASCII_NAK,
  SYMBOL_ASCII_SYN,
  SYMBOL_ASCII_ETB,
  SYMBOL_ASCII_CAN,
  SYMBOL_ASCII_EM,
  SYMBOL_ASCII_SUB,
  SYMBOL_ASCII_ESC,
  SYMBOL_ASCII_FS,
  SYMBOL_ASCII_GS,
  SYMBOL_ASCII_RS,
  SYMBOL_ASCII_US,

  SYMBOL_ASCII_SPACE,
  SYMBOL_ASCII_EXCLAMATION,
  SYMBOL_ASCII_DOUBLEQUOTE,
  SYMBOL_ASCII_NUMBER,
  SYMBOL_ASCII_DOLLAR,
  SYMBOL_ASCII_PERCENT,
  SYMBOL_ASCII_AMPERSAND,
  SYMBOL_ASCII_SINGLEQUOTE,
  SYMBOL_ASCII_LPAREN,
  SYMBOL_ASCII_RPAREN,
  SYMBOL_ASCII_ASTERISK,
  SYMBOL_ASCII_PLUS,
  SYMBOL_ASCII_COMMA,
  SYMBOL_ASCII_HYPHEN,
  SYMBOL_ASCII_PERIOD,
  SYMBOL_ASCII_SLASH,
  SYMBOL_ASCII_0,
  SYMBOL_ASCII_1,
  SYMBOL_ASCII_2,
  SYMBOL_ASCII_3,
  SYMBOL_ASCII_4,
  SYMBOL_ASCII_5,
  SYMBOL_ASCII_6,
  SYMBOL_ASCII_7,
  SYMBOL_ASCII_8,
  SYMBOL_ASCII_9,
  SYMBOL_ASCII_COLON,
  SYMBOL_ASCII_SEMICOLON,
  SYMBOL_ASCII_LANGLE,
  SYMBOL_ASCII_EQUAL,
  SYMBOL_ASCII_RANGLE,
  SYMBOL_ASCII_QUESTION,
  SYMBOL_ASCII_AT,
  SYMBOL_ASCII_A,
  SYMBOL_ASCII_B,
  SYMBOL_ASCII_C,
  SYMBOL_ASCII_D,
  SYMBOL_ASCII_E,
  SYMBOL_ASCII_F,
  SYMBOL_ASCII_G,
  SYMBOL_ASCII_H,
  SYMBOL_ASCII_I,
  SYMBOL_ASCII_J,
  SYMBOL_ASCII_K,
  SYMBOL_ASCII_L,
  SYMBOL_ASCII_M,
  SYMBOL_ASCII_N,
  SYMBOL_ASCII_O,
  SYMBOL_ASCII_P,
  SYMBOL_ASCII_Q,
  SYMBOL_ASCII_R,
  SYMBOL_ASCII_S,
  SYMBOL_ASCII_T,
  SYMBOL_ASCII_U,
  SYMBOL_ASCII_V,
  SYMBOL_ASCII_W,
  SYMBOL_ASCII_X,
  SYMBOL_ASCII_Y,
  SYMBOL_ASCII_Z,
  SYMBOL_ASCII_LBRACKET,
  SYMBOL_ASCII_BACKSLASH,
  SYMBOL_ASCII_RBRACKET,
  SYMBOL_ASCII_CARET,
  SYMBOL_ASCII_UNDERSCORE,
  SYMBOL_ASCII_BACKTICK,
  SYMBOL_ASCII_a,
  SYMBOL_ASCII_b,
  SYMBOL_ASCII_c,
  SYMBOL_ASCII_d,
  SYMBOL_ASCII_e,
  SYMBOL_ASCII_f,
  SYMBOL_ASCII_g,
  SYMBOL_ASCII_h,
  SYMBOL_ASCII_i,
  SYMBOL_ASCII_j,
  SYMBOL_ASCII_k,
  SYMBOL_ASCII_l,
  SYMBOL_ASCII_m,
  SYMBOL_ASCII_n,
  SYMBOL_ASCII_o,
  SYMBOL_ASCII_p,
  SYMBOL_ASCII_q,
  SYMBOL_ASCII_r,
  SYMBOL_ASCII_s,
  SYMBOL_ASCII_t,
  SYMBOL_ASCII_u,
  SYMBOL_ASCII_v,
  SYMBOL_ASCII_w,
  SYMBOL_ASCII_x,
  SYMBOL_ASCII_y,
  SYMBOL_ASCII_z,
  SYMBOL_ASCII_LBRACE,
  SYMBOL_ASCII_PIPE,
  SYMBOL_ASCII_RBRACE,
  SYMBOL_ASCII_TILDE,
  SYMBOL_ASCII_DELETE,

  SYMBOL_NONE,
  SYMBOL_SHIFT,
  SYMBOL_ALT,
  SYMBOL_CTRL,

  SYMBOL_CAPSLOCK,
  SYMBOL_NUMLOCK,

  SYMBOL_UP,
  SYMBOL_DOWN,
  SYMBOL_LEFT,
  SYMBOL_RIGHT,

  SYMBOL_COUNT,
} symbol_t;

/* Lookup table that maps a modifier state to an array of symbols. */
static symbol_t* modifier_state_to_symbols[MODIFIER_STATES];

typedef void (*action_t) (symbol_t, bool);
static action_t charset[SYMBOL_COUNT];

/* Insert a modifier group. */
static void
insert_modifier_group (unsigned int modifier)
{
  if (modifier_state_to_symbols[modifier] != 0) {
    syslog ("kb_us_104: error: modifier group exists");
    exit ();
  }
  /* TODO:  If these are sparse, it might be better to use a list. */
  modifier_state_to_symbols[modifier] = malloc (TOTAL_KEYS * sizeof (symbol_t));
  for (size_t key = 0; key != TOTAL_KEYS; ++key) {
    modifier_state_to_symbols[modifier][key] = SYMBOL_NONE;
  }
}

/* Insert an action into the table. */
static void
insert_symbol (unsigned int modifier,
	       unsigned char key,
	       symbol_t symbol)
{
  if (modifier_state_to_symbols[modifier] == 0) {
    syslog ("kb_us_104: error: modifier group does not exist");
    exit ();
  }
  modifier_state_to_symbols[modifier][key] = symbol;
}

/* Insert an action into all groups. */
static void
insert_symbol_all (unsigned char key,
		   symbol_t symbol)
{
  for (unsigned int modifier = 0; modifier != MODIFIER_STATES; ++modifier) {
    if (modifier_state_to_symbols[modifier] != 0) {
      modifier_state_to_symbols[modifier][key] = symbol;
    }
  }
}

static symbol_t
lookup_symbol (unsigned char key)
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

  if (modifier_state_to_symbols[modifier] != 0) {
    return modifier_state_to_symbols[modifier][key];
  }
  else {
    return SYMBOL_NONE;
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
process_key_code (unsigned char key_code,
		  bool make)
{
  if (key_code == 0) {
    syslog ("kb_us_104: warning: null key code");
  }

  symbol_t symbol = lookup_symbol (key_code);
  if (charset[symbol] != 0) {
    charset[symbol](symbol, make);
  }
}

static void
shift_func(symbol_t symbol,
	   bool make)
{
  if (make) {
    ++shift;
  }
  else {
    --shift;
  }
}

static void
alt_func(symbol_t symbol,
	 bool make)
{
  if (make) {
    ++alt;
  }
  else {
    --alt;
  }
}

static void
ctrl_func(symbol_t symbol,
	  bool make)
{
  if (make) {
    ++ctrl;
  }
  else {
    --ctrl;
  }
}

static void
ascii_func (symbol_t symbol,
	    bool make)
{
  if (make) {
    char c = symbol;
    if (caps) {
      if (c >= 'a' && c <= 'z') {
	c -= 32;
      }
      else if (c >= 'A' && c <= 'Z') {
	c += 32;
      }
    }

    if (buffer_file_put (&output_buffer, c) == -1) {
      syslog ("kb_us_104: errro: Could not write to output buffer");
      exit ();
    }
  }
}

static void
capslock_func (symbol_t symbol,
	       bool make)
{
  if (make) {
    caps = !caps;
  }
}

static void
numlock_func (symbol_t symbol,
	      bool make)
{
  if (make) {
    num = !num;
  }
}

static void
up_func (symbol_t symbol,
	 bool make)
{
  if (make) {
    if (buffer_file_puts (&output_buffer, "\e[A") == -1) {
      syslog ("kb_us_104: errro: Could not write to output buffer");
      exit ();
    }
  }
}

static void
down_func (symbol_t symbol,
	   bool make)
{
  if (make) {
    if (buffer_file_puts (&output_buffer, "\e[B") == -1) {
      syslog ("kb_us_104: errro: Could not write to output buffer");
      exit ();
    }
  }
}

static void
left_func (symbol_t symbol,
	   bool make)
{
  if (make) {
    if (buffer_file_puts (&output_buffer, "\e[D") == -1) {
      syslog ("kb_us_104: errro: Could not write to output buffer");
      exit ();
    }
  }
}

static void
right_func (symbol_t symbol,
	    bool make)
{
  if (make) {
    if (buffer_file_puts (&output_buffer, "\e[C") == -1) {
      syslog ("kb_us_104: errro: Could not write to output buffer");
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

    /* Connect the modifier keys to the modifier symbols. */
    insert_symbol_all (KEY_LSHIFT, SYMBOL_SHIFT);
    insert_symbol_all (KEY_RSHIFT, SYMBOL_SHIFT);
    insert_symbol_all (KEY_LALT, SYMBOL_ALT);
    insert_symbol_all (KEY_RALT, SYMBOL_ALT);
    insert_symbol_all (KEY_LCTRL, SYMBOL_CTRL);
    insert_symbol_all (KEY_RCTRL, SYMBOL_CTRL);

    /* Connect keys with ASCII symbols to their symbols. */
    insert_symbol (CTRL_WEIGHT, KEY_0, SYMBOL_ASCII_NULL);
    insert_symbol (CTRL_WEIGHT, KEY_A, SYMBOL_ASCII_SOH);
    insert_symbol (CTRL_WEIGHT, KEY_B, SYMBOL_ASCII_STX);
    insert_symbol (CTRL_WEIGHT, KEY_C, SYMBOL_ASCII_ETX);
    insert_symbol (CTRL_WEIGHT, KEY_D, SYMBOL_ASCII_EOT);
    insert_symbol (CTRL_WEIGHT, KEY_E, SYMBOL_ASCII_ENQ);
    insert_symbol (CTRL_WEIGHT, KEY_F, SYMBOL_ASCII_ACK);
    insert_symbol (CTRL_WEIGHT, KEY_G, SYMBOL_ASCII_BEL);
    insert_symbol (CTRL_WEIGHT, KEY_H, SYMBOL_ASCII_BS);
    insert_symbol (CTRL_WEIGHT, KEY_I, SYMBOL_ASCII_HT);
    insert_symbol (0, KEY_TAB, SYMBOL_ASCII_HT);
    insert_symbol (CTRL_WEIGHT, KEY_J, SYMBOL_ASCII_LF);
    insert_symbol (0, KEY_ENTER, SYMBOL_ASCII_LF);
    insert_symbol (CTRL_WEIGHT, KEY_K, SYMBOL_ASCII_VT);
    insert_symbol (CTRL_WEIGHT, KEY_L, SYMBOL_ASCII_FF);
    insert_symbol (CTRL_WEIGHT, KEY_M, SYMBOL_ASCII_CR);
    insert_symbol (CTRL_WEIGHT, KEY_N, SYMBOL_ASCII_SO);
    insert_symbol (CTRL_WEIGHT, KEY_O, SYMBOL_ASCII_SI);
    insert_symbol (CTRL_WEIGHT, KEY_P, SYMBOL_ASCII_DLE);
    insert_symbol (CTRL_WEIGHT, KEY_Q, SYMBOL_ASCII_DC1);
    insert_symbol (CTRL_WEIGHT, KEY_R, SYMBOL_ASCII_DC2);
    insert_symbol (CTRL_WEIGHT, KEY_S, SYMBOL_ASCII_DC3);
    insert_symbol (CTRL_WEIGHT, KEY_T, SYMBOL_ASCII_DC4);
    insert_symbol (CTRL_WEIGHT, KEY_U, SYMBOL_ASCII_NAK);
    insert_symbol (CTRL_WEIGHT, KEY_V, SYMBOL_ASCII_SYN);
    insert_symbol (CTRL_WEIGHT, KEY_W, SYMBOL_ASCII_ETB);
    insert_symbol (CTRL_WEIGHT, KEY_X, SYMBOL_ASCII_CAN);
    insert_symbol (CTRL_WEIGHT, KEY_Y, SYMBOL_ASCII_EM);
    insert_symbol (CTRL_WEIGHT, KEY_Z, SYMBOL_ASCII_SUB);
    insert_symbol (0, KEY_ESCAPE, SYMBOL_ASCII_ESC);
    insert_symbol (CTRL_WEIGHT, KEY_1, SYMBOL_ASCII_FS);
    insert_symbol (CTRL_WEIGHT, KEY_2, SYMBOL_ASCII_GS);
    insert_symbol (CTRL_WEIGHT, KEY_3, SYMBOL_ASCII_RS);
    insert_symbol (CTRL_WEIGHT, KEY_4, SYMBOL_ASCII_US);
    
    insert_symbol (0, KEY_SPACE, SYMBOL_ASCII_SPACE);
    insert_symbol (SHIFT_WEIGHT, KEY_SPACE, SYMBOL_ASCII_SPACE);
    insert_symbol (SHIFT_WEIGHT, KEY_1, SYMBOL_ASCII_EXCLAMATION);
    insert_symbol (SHIFT_WEIGHT, KEY_QUOTE, SYMBOL_ASCII_DOUBLEQUOTE);
    insert_symbol (SHIFT_WEIGHT, KEY_3, SYMBOL_ASCII_NUMBER);
    insert_symbol (SHIFT_WEIGHT, KEY_4, SYMBOL_ASCII_DOLLAR);
    insert_symbol (SHIFT_WEIGHT, KEY_5, SYMBOL_ASCII_PERCENT);
    insert_symbol (SHIFT_WEIGHT, KEY_7, SYMBOL_ASCII_AMPERSAND);
    insert_symbol (0, KEY_QUOTE, SYMBOL_ASCII_SINGLEQUOTE);
    insert_symbol (SHIFT_WEIGHT, KEY_9, SYMBOL_ASCII_LPAREN);
    insert_symbol (SHIFT_WEIGHT, KEY_0, SYMBOL_ASCII_RPAREN);
    insert_symbol (SHIFT_WEIGHT, KEY_8, SYMBOL_ASCII_ASTERISK);
    insert_symbol (SHIFT_WEIGHT, KEY_EQUAL, SYMBOL_ASCII_PLUS);
    insert_symbol (0, KEY_COMMA, SYMBOL_ASCII_COMMA);
    insert_symbol (0, KEY_MINUS, SYMBOL_ASCII_HYPHEN);
    insert_symbol (0, KEY_PERIOD, SYMBOL_ASCII_PERIOD);
    insert_symbol (0, KEY_SLASH, SYMBOL_ASCII_SLASH);
    insert_symbol (0, KEY_0, SYMBOL_ASCII_0);
    insert_symbol (0, KEY_1, SYMBOL_ASCII_1);
    insert_symbol (0, KEY_2, SYMBOL_ASCII_2);
    insert_symbol (0, KEY_3, SYMBOL_ASCII_3);
    insert_symbol (0, KEY_4, SYMBOL_ASCII_4);
    insert_symbol (0, KEY_5, SYMBOL_ASCII_5);
    insert_symbol (0, KEY_6, SYMBOL_ASCII_6);
    insert_symbol (0, KEY_7, SYMBOL_ASCII_7);
    insert_symbol (0, KEY_8, SYMBOL_ASCII_8);
    insert_symbol (0, KEY_9, SYMBOL_ASCII_9);
    insert_symbol (SHIFT_WEIGHT, KEY_SEMICOLON, SYMBOL_ASCII_COLON);
    insert_symbol (0, KEY_SEMICOLON, SYMBOL_ASCII_SEMICOLON);
    insert_symbol (SHIFT_WEIGHT, KEY_COMMA, SYMBOL_ASCII_LANGLE);
    insert_symbol (0, KEY_EQUAL, SYMBOL_ASCII_EQUAL);
    insert_symbol (SHIFT_WEIGHT, KEY_PERIOD, SYMBOL_ASCII_RANGLE);
    insert_symbol (SHIFT_WEIGHT, KEY_SLASH, SYMBOL_ASCII_QUESTION);
    insert_symbol (SHIFT_WEIGHT, KEY_2, SYMBOL_ASCII_AT);
    insert_symbol (SHIFT_WEIGHT, KEY_A, SYMBOL_ASCII_A);
    insert_symbol (SHIFT_WEIGHT, KEY_B, SYMBOL_ASCII_B);
    insert_symbol (SHIFT_WEIGHT, KEY_C, SYMBOL_ASCII_C);
    insert_symbol (SHIFT_WEIGHT, KEY_D, SYMBOL_ASCII_D);
    insert_symbol (SHIFT_WEIGHT, KEY_E, SYMBOL_ASCII_E);
    insert_symbol (SHIFT_WEIGHT, KEY_F, SYMBOL_ASCII_F);
    insert_symbol (SHIFT_WEIGHT, KEY_G, SYMBOL_ASCII_G);
    insert_symbol (SHIFT_WEIGHT, KEY_H, SYMBOL_ASCII_H);
    insert_symbol (SHIFT_WEIGHT, KEY_I, SYMBOL_ASCII_I);
    insert_symbol (SHIFT_WEIGHT, KEY_J, SYMBOL_ASCII_J);
    insert_symbol (SHIFT_WEIGHT, KEY_K, SYMBOL_ASCII_K);
    insert_symbol (SHIFT_WEIGHT, KEY_L, SYMBOL_ASCII_L);
    insert_symbol (SHIFT_WEIGHT, KEY_M, SYMBOL_ASCII_M);
    insert_symbol (SHIFT_WEIGHT, KEY_N, SYMBOL_ASCII_N);
    insert_symbol (SHIFT_WEIGHT, KEY_O, SYMBOL_ASCII_O);
    insert_symbol (SHIFT_WEIGHT, KEY_P, SYMBOL_ASCII_P);
    insert_symbol (SHIFT_WEIGHT, KEY_Q, SYMBOL_ASCII_Q);
    insert_symbol (SHIFT_WEIGHT, KEY_R, SYMBOL_ASCII_R);
    insert_symbol (SHIFT_WEIGHT, KEY_S, SYMBOL_ASCII_S);
    insert_symbol (SHIFT_WEIGHT, KEY_T, SYMBOL_ASCII_T);
    insert_symbol (SHIFT_WEIGHT, KEY_U, SYMBOL_ASCII_U);
    insert_symbol (SHIFT_WEIGHT, KEY_V, SYMBOL_ASCII_V);
    insert_symbol (SHIFT_WEIGHT, KEY_W, SYMBOL_ASCII_W);
    insert_symbol (SHIFT_WEIGHT, KEY_X, SYMBOL_ASCII_X);
    insert_symbol (SHIFT_WEIGHT, KEY_Y, SYMBOL_ASCII_Y);
    insert_symbol (SHIFT_WEIGHT, KEY_Z, SYMBOL_ASCII_Z);
    insert_symbol (0, KEY_LBRACKET, SYMBOL_ASCII_LBRACKET);
    insert_symbol (0, KEY_BACKSLASH, SYMBOL_ASCII_BACKSLASH);
    insert_symbol (0, KEY_RBRACKET, SYMBOL_ASCII_RBRACKET);
    insert_symbol (SHIFT_WEIGHT, KEY_6, SYMBOL_ASCII_CARET);
    insert_symbol (SHIFT_WEIGHT, KEY_MINUS, SYMBOL_ASCII_UNDERSCORE);
    insert_symbol (0, KEY_BACKTICK, SYMBOL_ASCII_BACKTICK);
    insert_symbol (0, KEY_A, SYMBOL_ASCII_a);
    insert_symbol (0, KEY_B, SYMBOL_ASCII_b);
    insert_symbol (0, KEY_C, SYMBOL_ASCII_c);
    insert_symbol (0, KEY_D, SYMBOL_ASCII_d);
    insert_symbol (0, KEY_E, SYMBOL_ASCII_e);
    insert_symbol (0, KEY_F, SYMBOL_ASCII_f);
    insert_symbol (0, KEY_G, SYMBOL_ASCII_g);
    insert_symbol (0, KEY_H, SYMBOL_ASCII_h);
    insert_symbol (0, KEY_I, SYMBOL_ASCII_i);
    insert_symbol (0, KEY_J, SYMBOL_ASCII_j);
    insert_symbol (0, KEY_K, SYMBOL_ASCII_k);
    insert_symbol (0, KEY_L, SYMBOL_ASCII_l);
    insert_symbol (0, KEY_M, SYMBOL_ASCII_m);
    insert_symbol (0, KEY_N, SYMBOL_ASCII_n);
    insert_symbol (0, KEY_O, SYMBOL_ASCII_o);
    insert_symbol (0, KEY_P, SYMBOL_ASCII_p);
    insert_symbol (0, KEY_Q, SYMBOL_ASCII_q);
    insert_symbol (0, KEY_R, SYMBOL_ASCII_r);
    insert_symbol (0, KEY_S, SYMBOL_ASCII_s);
    insert_symbol (0, KEY_T, SYMBOL_ASCII_t);
    insert_symbol (0, KEY_U, SYMBOL_ASCII_u);
    insert_symbol (0, KEY_V, SYMBOL_ASCII_v);
    insert_symbol (0, KEY_W, SYMBOL_ASCII_w);
    insert_symbol (0, KEY_X, SYMBOL_ASCII_x);
    insert_symbol (0, KEY_Y, SYMBOL_ASCII_y);
    insert_symbol (0, KEY_Z, SYMBOL_ASCII_z);
    insert_symbol (SHIFT_WEIGHT, KEY_LBRACKET, SYMBOL_ASCII_LBRACE);
    insert_symbol (SHIFT_WEIGHT, KEY_BACKSLASH, SYMBOL_ASCII_PIPE);
    insert_symbol (SHIFT_WEIGHT, KEY_RBRACKET, SYMBOL_ASCII_RBRACE);
    insert_symbol (SHIFT_WEIGHT, KEY_BACKTICK, SYMBOL_ASCII_TILDE);

    insert_symbol (0, KEY_BACKSPACE, SYMBOL_ASCII_DELETE);

    /* Caps Lock. */
    insert_symbol_all (KEY_CAPSLOCK, SYMBOL_CAPSLOCK);

    /* Num Lock. */
    insert_symbol_all (KEY_NUMLOCK, SYMBOL_NUMLOCK);

    /* Arrow keys. */
    insert_symbol (0, KEY_UP, SYMBOL_UP);
    insert_symbol (0, KEY_DOWN, SYMBOL_DOWN);
    insert_symbol (0, KEY_LEFT, SYMBOL_LEFT);
    insert_symbol (0, KEY_RIGHT, SYMBOL_RIGHT);

    /* Create a character set. */

    
    /* ASCII character map. */
    for (symbol_t sym = SYMBOL_ASCII_NULL; sym <= SYMBOL_ASCII_DELETE; ++sym) {
      charset[sym] = ascii_func;
    }

    /* Insert modifying actions. */
    charset[SYMBOL_SHIFT] = shift_func;
    charset[SYMBOL_ALT] = alt_func;
    charset[SYMBOL_CTRL] = ctrl_func;

    /* Caps Lock. */
    charset[SYMBOL_CAPSLOCK] = capslock_func;

    /* Num Lock. */
    charset[SYMBOL_NUMLOCK] = numlock_func;

    /* Cursor movement. */
    charset[SYMBOL_UP] = up_func;
    charset[SYMBOL_DOWN] = down_func;
    charset[SYMBOL_LEFT] = left_func;
    charset[SYMBOL_RIGHT] = right_func;
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
    if (scan < BREAK_MASK) {
      if (!escaped) {
	process_key_code (scan_to_key[scan], true);
      }
      else {
	escaped = false;
	/* Ignore fake shifts. */
	if (scan != SCAN_LSHIFT) {
	  process_key_code (escaped_scan_to_key[scan], true);
	}
      }
    }
    else if (scan == 0xE0) {
      escaped = true;
    }
    else if (scan == 0xE1) {
      process_key_code (KEY_PAUSE, true);
      process_key_code (KEY_PAUSE, false);
      consume = 5;
    }
    else {
      /* Break. */
      scan -= BREAK_MASK;
      if (!escaped) {
	process_key_code (scan_to_key[scan], false);
      }
      else {
	escaped = false;
	/* Ignore fake shifts. */
	if (scan != SCAN_LSHIFT) {
	  process_key_code (escaped_scan_to_key[scan], false);
	}
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
