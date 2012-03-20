#include "terminal.h"
#include <automaton.h>
#include <dymem.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <buffer_file.h>
#include "vga_msg.h"
#include "mouse_msg.h"
#include "syslog.h"

/*
  Terminal
  ========
  
  The terminal allows physical input and output devices to be shared among multiple clients.
  One client is designated as the active client, i.e., the one with the "focus."
  The terminal routes keyboard and mouse data to the active client and routes output from the active client to the VGA hardware.
  Other devices can be supported if there is interest.
  The focus is changed by interpretting special sequences from the keyboard.

  Scan Code Translation
  ---------------------
  The terminal delivers both the raw scan codes and ASCII text from the keyboard.
  The ultimate goal should be to support different mappings and characters sets like the Linux keyboard driver.

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

  VGA Terminal
  ------------
  The server interprets a stream of a data and control sequences and renders them to a VGA.
  The goal is that the terminal server will conform to the ECMA-48 Standard.
  ECMA-48 specifies how control functions are encoded and interpretted.
  Conforming to this standard does not mean implementing every control function.
  Rather confomances means:
  (1) a control sequences specified in the standard is always interpretted as a control sequences;
  (2) a function specified in the standard must always be code by the representation in the standard;
  (3) reserved functions are ignored; and
  (4) functions that are supported must be identified.
  See Section 2 of the ECMA-48 Standard for details.

  Caveats
  -------
  Data must be encoded using 7-bit ASCII.
  This is not without purpose as we can support UTF-8.
    
  Limitations
  -----------
  The server assumes an 80x25 single page display.
  The character progression is assumed left-to-right and the line progression is assumed top-to-bottom.

  Authors
  -------
  Justin R. Wilson

  Status
  ------
  + indicates the function is implemented.
  LF feed is not standard as it is actually CR+LF.
  Tabs are assumed at every eighth position.

  ACK  - ACKNOWLEDGE
  APC  - APPLICATION PROGRAM COMMAND
  BEL  - BELL
  BPH  - BREAK PERMITTED HERE
  + BS   - BACKSPACE
  CAN  - CANCEL
  CBT  - CURSOR BACKWARD TABULATION
  CCH  - CANCEL CHARACTER
  CHA  - CURSOR CHARACTER ABSOLUTE
  CHT  - CURSOR FORWARD TABULATION
  CMD  - CODING METHOD DELIMITER
  CNL  - CURSOR NEXT LINE
  CPL  - CURSOR PRECEDING LINE
  CPR  - ACTIVE POSITION REPORT
  + CR   - CARRIAGE RETURN
  CSI  - CONTROL SEQUENCE INTRODUCER
  CTC  - CURSOR TABULATION CONTROL
  + CUB  - CURSOR LEFT
  + CUD  - CURSOR DOWN
  + CUF  - CURSOR RIGHT
  + CUP  - CURSOR POSITION
  + CUU  - CURSOR UP
  CVT  - CURSOR LINE TABULATION
  DA   - DEVICE ATTRIBUTES
  DAQ  - DEFINE AREA QUALIFICATION
  DCH  - DELETE CHARACTER
  DCS  - DEVICE CONTROL STRING
  DC1  - DEVICE CONTROL ONE
  DC2  - DEVICE CONTROL TWO
  DC3  - DEVICE CONTROL THREE
  DC4  - DEVICE CONTROL FOUR
  DL   - DELETE LINE
  DLE  - DATA LINK ESCAPE
  DMI  - DISABLE MANUAL INPUT
  DSR  - DEVICE STATUS REPORT
  DTA  - DIMENSION TEXT AREA
  EA   - ERASE IN AREA
  ECH  - ERASE CHARACTER
  + ED   - ERASE IN PAGE
  EF   - ERASE IN FIELD
  EL   - ERASE IN LINE
  EM   - END OF MEDIUM
  EMI  - ENABLE MANUAL INPUT
  ENQ  - ENQUIRY
  EOT  - END OF TRANSMISSION
  EPA  - END OF GUARDED AREA
  ESA  - END OF SELECTED AREA
  ESC  - ESCAPE
  ETB  - END OF TRANSMISSION BLOCK
  ETX  - END OF TEXT
  FF   - FORM FEED
  FNK  - FUNCTION KEY
  FNT  - FONT SELECTION
  GCC  - GRAPHIC CHARACTER COMBINATION
  GSM  - GRAPHIC SIZE MODIFICATION
  GSS  - GRAPHIC SIZE SELECTION
  HPA  - CHARACTER POSITION ABSOLUTE
  HPB  - CHARACTER POSITION BACKWARD
  HPR  - CHARACTER POSITION FORWARD
  + HT   - CHARACTER TABULATION
  HTJ  - CHARACTER TABULATION WITH JUSTIFICATION
  HTS  - CHARACTER TABULATION SET
  HVP  - CHARACTER AND LINE POSITION
  + ICH  - INSERT CHARACTER
  IDCS - IDENTIFY DEVICE CONTROL STRING
  IG   - IDENTIFY GRAPHIC SUBREPERTOIRE
  IL   - INSERT LINE
  INT  - INTERRUPT
  IS1  - INFORMATION SEPARATOR ONE (US - UNIT SEPARATOR)
  IS2  - INFORMATION SEPARATOR TWO (RS - RECORD SEPARATOR)
  IS3  - INFORMATION SEPARATOR THREE (GS - GROUP SEPARATOR)
  IS4  - INFORMATION SEPARATOR FOUR (FS - FILE SEPARATOR)
  JFY  - JUSTIFY
  + LF   - LINE FEED
  LS0  - LOCKING-SHIFT ZERO
  LS1  - LOCKING-SHIFT ONE
  LS1R - LOCKING-SHIFT ONE RIGHT
  LS2  - LOCKING-SHIFT TWO
  LS2R - LOCKING-SHIFT TWO RIGHT
  LS3  - LOCKING-SHIFT THREE
  LS3R - LOCKING-SHIFT THREE RIGHT
  MC   - MEDIA COPY
  MW   - MESSAGE WAITING
  NAK  - NEGATIVE ACKNOWLEDGE
  NBH  - NO BREAK HERE
  NEL  - NEXT LINE
  NP   - NEXT PAGE
  + NUL  - NULL
  OSC  - OPERATING SYSTEM COMMAND
  PEC  - PRESENTATION EXPAND OR CONTRACT
  PFS  - PAGE FORMAT SELECTION
  PLD  - PARTIAL LINE FORWARD
  PLU  - PARTIAL LINE BACKWARD
  PM   - PRIVACY MESSAGE
  PP   - PRECEDING PAGE
  PPA  - PAGE POSITION ABSOLUTE
  PPB  - PAGE POSITION BACKWARD
  PPR  - PAGE POSITION FORWARD
  PTX  - PARALLEL TEXTS
  PU1  - PRIVATE USE ONE
  PU2  - PRIVATE USE TWO
  QUAT - QUAD
  REP  - REPEAT
  RI   - REVERSE LINE FEED
  RIS  - RESET TO INITIAL STATE
  RM   - RESET MODE
  SACS - SET ADDITIONAL CHARACTER SEPARATION
  SAPV - SELECT ALTERNATIVE REPRESENTATION VARIANTS
  SCI  - SINGLE CHARACTER INTRODUCER
  SCO  - SELECT CHARACTER ORIENTATION
  SCP  - SELECT CHARACTER PATH
  SCS  - SET CHARACTER SPACING
  SD   - SCROLL DOWN
  SDS  - START DIRECTED STRING
  SEE  - SELECT EDITING EXTENT
  SEF  - SHEET EJECT AND FEED
  SGR  - SELECT GRAPHIC RENDITION
  SHS  - SELECT CHARACTER SPACING
  SI   - SHIFT-IN
  SIMD - SELECT IMPLICIT MOVEMENT DIRECTION
  SL   - SCROLL LEFT
  SLH  - SET LINE HOME
  SLL  - SET LINE LIMIT
  SLS  - SET LINE SPACING
  SM   - SET MODE
  SO   - SHIFT-OUT
  SOH  - START OF HEADING
  SOS  - START OF STRING
  SPA  - START OF GUARDED AREA
  SPD  - SELECT PRESENTATION DIRECTIONS
  SPH  - SET PAGE HOME
  SPI  - SPACING INCREMENT
  SPL  - SET PAGE LIMIT
  SPQR - SELECT PRINT QUALITY AND RAPIDITY
  SR   - SCROLL RIGHT
  SRCS - SET REDUCED CHARACTER SEPARATION
  SRS  - START REVERSED STRING
  SSA  - START OF SELECTED AREA
  SSU  - SELECTED SIZE UNIT
  SSW  - SET SPACE WIDTH
  SS2  - SINGLE-SHIFT TWO
  SS3  - SINGLE-SHIFT THREE
  ST   - STRING TERMINATOR
  STAB - SELECTIVE TABULATION
  STS  - SET TRANSMIT STATE
  STX  - START OF TEXT
  SU   - SCROLL UP
  SUB  - SUBSTITUTE
  SVS  - SELECT LINE SPACING
  SYN  - SYNCHRONOUS IDLE
  TAC  - TABULATION ALIGNED CENTERED
  TALE - TABULATION ALIGNED LEADING EDGE
  TATE - TABULATION ALIGNED TRAILING EDGE
  TBC  - TABULATION CLEAR
  TCC  - TABULATION CENTERED ON CHARACTER
  TSR  - TABULATION STOP REMOVE
  TSS  - THIN SPACE SPECIFICATION
  VPA  - LINE POSITION ABSOLUTE
  VPB  - LINE POSITION BACKWARD
  VPR  - LINE POSITION FORWARD
  VT   - LINE TABULATION
  VTS  - LINE TABULATION SET
*/

#define PARAMETER_SIZE 2

typedef enum {
  NORMAL,
  ESCAPED,
  CONTROL
} mode_t;

typedef struct client client_t;
struct client {
  /* Associated aid. */
  aid_t aid;

  /* Keyboard. */
  bd_t scan_codes_bd;
  buffer_file_t scan_codes_buffer;
  bd_t ascii_bd;
  buffer_file_t ascii_buffer;

  /* Mouse. */
  bd_t mouse_packets_bd;
  mouse_packet_list_t mouse_packet_list;
  
  /* Buffer for the screen data. */
  bd_t screen_bd;
  /* Same only mapped in. */
  unsigned short* screen_buffer;
  /* Active position.  These are zero based. */
  unsigned short active_position_x;
  unsigned short active_position_y;

  /* Variables for interpretting control sequences. */
  mode_t mode;
  int parameter[PARAMETER_SIZE];
  size_t parameter_count;

  /* Next on the list. */
  client_t* next;
};

/* The list of clients. */
static client_t* client_list_head = 0;
/* The active client. */
static client_t* active_client = 0;

/* Buffers to drive the vga. */
static bd_t vga_op_list_bda = -1;
static bd_t vga_op_list_bdb = -1;
static vga_op_list_t vga_op_list;

#define LINE_HOME_POSITION 0
#define LINE_LIMIT_POSITION 80
#define PAGE_HOME_POSITION 0
#define PAGE_LIMIT_POSITION 25

/*
  Size of a character cell.
  1 byte contains the character.
  1 byte contains attributes.
*/
#define CELL_SIZE 2

static void
switch_to_client (client_t* client)
{
  if (client != active_client) {
    active_client = client;
    if (active_client != 0) {
      /* Send the data. */
      if (vga_op_list_write_bassign (&vga_op_list, VGA_TEXT_MEMORY_BEGIN, PAGE_LIMIT_POSITION * LINE_LIMIT_POSITION * CELL_SIZE, active_client->screen_bd) != 0) {
      	syslog ("terminal: Could not write vga op list");
      	exit ();
      }

      /* Send the cursor. */
      if (vga_op_list_write_set_cursor_location (&vga_op_list, active_client->active_position_y * LINE_LIMIT_POSITION + active_client->active_position_x) != 0) {
      	syslog ("terminal: Could not write vga op list");
      	exit ();
      }
    }
  }
}

#define INIT_NO 1
#define STOP_NO 2
#define SYSLOG_NO 3
#define SCAN_CODES_IN_NO 4
#define MOUSE_PACKETS_IN_NO 5
#define SCAN_CODES_OUT_NO 6
#define STDOUT_NO 7
#define MOUSE_PACKETS_OUT_NO 8
#define STDIN_NO 9
#define VGA_OP_NO 10
#define DESTROYED_NO 11

#define WARNING "terminal: warning: "
#define ERROR "terminal: error: "

/*********************************
 * SCAN CODE TRANSLATION SECTION *
 *********************************/

/* Mask for break codes. */
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

  SYMBOL_NEXT_CLIENT,

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
  if (lshift) {
    modifier |= LSHIFT_WEIGHT;
  }
  if (rshift) {
    modifier |= RSHIFT_WEIGHT;
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

  if (modifier_state_to_symbols[modifier] != 0) {
    return modifier_state_to_symbols[modifier][key];
  }
  else {
    return SYMBOL_NONE;
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
shift_func (symbol_t symbol,
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
alt_func (symbol_t symbol,
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
ctrl_func (symbol_t symbol,
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

    if (active_client != 0) {
      if (buffer_file_put (&active_client->ascii_buffer, c) != 0) {
	syslog ("kb_us_104: errro: Could not write to output buffer");
	exit ();
      }
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
    if (active_client != 0) {
      if (buffer_file_puts (&active_client->ascii_buffer, "\e[A") != 0) {
	syslog ("kb_us_104: errro: Could not write to output buffer");
	exit ();
      }
    }
  }
}

static void
down_func (symbol_t symbol,
	   bool make)
{
  if (make) {
    if (active_client != 0) {
      if (buffer_file_puts (&active_client->ascii_buffer, "\e[B") != 0) {
	syslog ("kb_us_104: errro: Could not write to output buffer");
	exit ();
      }
    }
  }
}

static void
left_func (symbol_t symbol,
	   bool make)
{
  if (make) {
    if (active_client != 0) {
      if (buffer_file_puts (&active_client->ascii_buffer, "\e[D") != 0) {
	syslog ("kb_us_104: errro: Could not write to output buffer");
	exit ();
      }
    }
  }
}

static void
right_func (symbol_t symbol,
	    bool make)
{
  if (make) {
    if (active_client != 0) {
      if (buffer_file_puts (&active_client->ascii_buffer, "\e[C") != 0) {
	syslog ("kb_us_104: errro: Could not write to output buffer");
	exit ();
      }
    }
  }
}

static void
next_client_func (symbol_t symbol,
		  bool make)
{
  if (make) {
    if (active_client != 0 && active_client->next != 0) {
      switch_to_client (active_client->next);
    }
    else {
      switch_to_client (client_list_head);
    }
  }
}


/* White on black for VGA. */
#define ATTRIBUTE 0x0F00

/* Initialization flag. */
static bool initialized = false;

typedef enum {
  RUN,
  STOP,
} state_t;
static state_t state = RUN;

/* Syslog buffer. */
static bd_t syslog_bd = -1;
static buffer_file_t syslog_buffer;

static client_t*
find_client (aid_t aid)
{
  client_t* ptr;
  for (ptr = client_list_head; ptr != 0 && ptr->aid != aid; ptr = ptr->next) ;;
  return ptr;
}

static client_t*
create_client (aid_t aid)
{
  client_t* client = malloc (sizeof (client_t));
  client->aid = aid;

  client->scan_codes_bd = buffer_create (0);
  if (client->scan_codes_bd == -1) {
    syslog ("terminal: Could not create scan codes buffer");
    exit ();
  }
  if (buffer_file_initw (&client->scan_codes_buffer, client->scan_codes_bd) != 0) {
    syslog ("terminal: Could not initialize scan codes buffer");
    exit ();
  }

  client->ascii_bd = buffer_create (0);
  if (client->ascii_bd == -1) {
    syslog ("terminal: Could not create ascii buffer");
    exit ();
  }
  if (buffer_file_initw (&client->ascii_buffer, client->ascii_bd) != 0) {
    syslog ("terminal: Could not initialize ascii buffer");
    exit ();
  }

  client->mouse_packets_bd = buffer_create (0);
  if (client->mouse_packets_bd == -1) {
    syslog ("terminal: Could not create mouse packet buffer");
    exit ();
  }
  if (mouse_packet_list_initw (&client->mouse_packet_list, client->mouse_packets_bd) != 0) {
    syslog ("terminal: Could not initialize mouse packet buffer");
    exit ();
  }

  client->screen_bd = buffer_create (size_to_pages (PAGE_LIMIT_POSITION * LINE_LIMIT_POSITION * CELL_SIZE));
  if (client->screen_bd == -1) {
    syslog ("terminal: Could not create client buffer");
    exit ();
  }
  client->screen_buffer = buffer_map (client->screen_bd);
  if (client->screen_buffer == 0) {
    syslog ("terminal: Could not map client buffer");
    exit ();
  }
  /* ECMA-48 states that the initial state of the characters must be "erased."
     We use spaces. */
  unsigned short* data = client->screen_buffer;
  for (size_t y = 0; y != PAGE_LIMIT_POSITION; ++y) {
    for (size_t x = 0; x != LINE_LIMIT_POSITION; ++x) {
      data[y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
    }
  }
  /* I don't recall ECMA-48 specifying the initial state of the active position but this seems reasonable. */
  client->active_position_x = LINE_HOME_POSITION;
  client->active_position_y = PAGE_HOME_POSITION;

  client->mode = NORMAL;

  client->next = client_list_head;
  client_list_head = client;

  if (active_client == 0) {
    switch_to_client (client);
  }

  if (subscribe_destroyed (aid, DESTROYED_NO) != 0) {
    syslog ("terminal: Could not subscribe");
    exit ();
  }

  return client;
}

static void
destroy_client (client_t* client)
{
  if (client == active_client) {
    /* Switch to the null client. */
    switch_to_client (0);
  }
  buffer_destroy (client->screen_bd);
  free (client);
}

static void
scroll (client_t* client)
{
  /* Move the data up one line. */
  memmove (client->screen_buffer, &client->screen_buffer[LINE_LIMIT_POSITION], (PAGE_LIMIT_POSITION - 1) * LINE_LIMIT_POSITION * CELL_SIZE);
  /* Clear the last line. */
  for (size_t x = 0; x != LINE_LIMIT_POSITION; ++x) {
    client->screen_buffer[(PAGE_LIMIT_POSITION - 1) * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
  }
  /* Change the active y. */
  --client->active_position_y;
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    syslog_bd = buffer_create (0);
    if (syslog_bd == -1) {
      /* Nothing we can do. */
      exit ();
    }
    if (buffer_file_initw (&syslog_buffer, syslog_bd) != 0) {
      /* Nothing we can do. */
      exit ();
    }

    vga_op_list_bda = buffer_create (0);
    vga_op_list_bdb = buffer_create (0);
    if (vga_op_list_bda == -1 ||
	vga_op_list_bdb == -1) {
      bfprintf (&syslog_buffer, ERROR "could not create vga op list buffers\n");
      state = STOP;
      return;
    }

    if (vga_op_list_initw (&vga_op_list, vga_op_list_bda, vga_op_list_bdb) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not initialize vga op list buffers\n");
      state = STOP;
      return;
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

    /* Previous and next client. */
    insert_symbol (CTRL_WEIGHT | ALT_WEIGHT, KEY_EQUAL, SYMBOL_NEXT_CLIENT);
    
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

    charset[SYMBOL_NEXT_CLIENT] = next_client_func;
  }
}

static void
process_normal (client_t* client,
		char c)
{
  /* Process a 7-bit ASCII character. */
  switch (c) {
  case NUL:
    /* Do nothing. */
    break;
  case BS:
    /* Backspace. */
    if (client->active_position_x != 0) {
      --client->active_position_x;
    }
    break;
  case HT:
    /* Horizontal tab. */
    client->active_position_x = (client->active_position_x + 8) & ~(8-1);
    break;
  case LF:
    /* Line feed.  (NOT STANDARD!) */
    /* Perform a carriage return. */
    client->active_position_x = 0;
    /* Then the line feed. */
    ++client->active_position_y;
    if (client->active_position_y == PAGE_LIMIT_POSITION) {
      scroll (client);
    }
    break;
  case CR:
    /* Carriage return. */
    client->active_position_x = 0;
    break;
  case ESC:
    /* Begin of escaped sequence. */
    client->mode = ESCAPED;
    break;
  case DEL:
    /* Rub-out the character under the cursor. */
    client->screen_buffer[client->active_position_y * LINE_LIMIT_POSITION + client->active_position_x] = ATTRIBUTE | ' ';
    break;
  default:
    /* ASCII character that can be displayed. */
    if (c >= ' ' && c <= '~') {
      client->screen_buffer[client->active_position_y * LINE_LIMIT_POSITION + client->active_position_x++] = ATTRIBUTE | c;
      
      if (client->active_position_x == LINE_LIMIT_POSITION) {
	/* Active position is at the end of the line.
	   ECMA-48 does not define behavior in this circumstance.
	   We will move to home and scroll.
	*/
	client->active_position_x = LINE_HOME_POSITION;
	++client->active_position_y;
      }
      
      if (client->active_position_y == PAGE_LIMIT_POSITION) {
	/* Same idea. */
	scroll (client);
      }
    }
    break;
  }
}

static void
process_escaped (client_t* client,
		 char c)
{
  switch (c) {
  case CSI:
    /* Prepare for parameters. */
    for (size_t idx = 0; idx != PARAMETER_SIZE; ++idx) {
      client->parameter[idx] = 0;
    }
    client->parameter_count = 0;
    client->mode = CONTROL;
    break;
  }
}

static void
process_control (client_t* client,
		 char c)
{
  switch (c) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    if (client->parameter_count == 0) {
      client->parameter_count = 1;
    }
    if (client->parameter_count <= PARAMETER_SIZE) {
      client->parameter[client->parameter_count - 1] *= 10;
      client->parameter[client->parameter_count - 1] += (c - '0');
    }
    break;
  case ';':
    ++client->parameter_count;
    break;
  case ICH:
    {
      /* Parse the parameters. */
      unsigned short count = 1;
      if (client->parameter_count >= 1) {
      	count = client->parameter[0];
      }
      /* Shift the line. */
      for (unsigned short dest_x = LINE_LIMIT_POSITION - 1; dest_x != client->active_position_x && dest_x >= count; --dest_x) {
	client->screen_buffer[client->active_position_y * LINE_LIMIT_POSITION + dest_x] =
	  client->screen_buffer[client->active_position_y * LINE_LIMIT_POSITION + dest_x - count];
      }
      /* Replace with spaces. */
      for (unsigned short dest_x = client->active_position_x; dest_x != LINE_LIMIT_POSITION && count != 0; ++dest_x, --count) {
	client->screen_buffer[client->active_position_y * LINE_LIMIT_POSITION + dest_x] = ATTRIBUTE | ' ';
      }

      /* Move to line home. */
      client->active_position_x = LINE_HOME_POSITION;


      /* Reset. */
      client->mode = NORMAL;
    }
    break;
  case CUU:
    {
      /* Parse the parameters. */
      unsigned short offset = 1;
      if (client->parameter_count >= 1) {
	offset = client->parameter[0];
      }
      /* Set the position if in bounds. */
      if (offset <= client->active_position_y) {
	client->active_position_y -= offset;
      }

      /* Reset. */
      client->mode = NORMAL;
    }
    break;
  case CUD:
    {
      /* Parse the parameters. */
      unsigned short offset = 1;
      if (client->parameter_count >= 1) {
	offset = client->parameter[0];
      }
      /* Set the position if in bounds. */
      if (client->active_position_y + offset < PAGE_LIMIT_POSITION) {
	client->active_position_y += offset;
      }

      /* Reset. */
      client->mode = NORMAL;
    }
    break;
  case CUF:
    {
      /* Parse the parameters. */
      unsigned short offset = 1;
      if (client->parameter_count >= 1) {
	offset = client->parameter[0];
      }
      /* Set the position if in bounds. */
      if (client->active_position_x + offset < LINE_LIMIT_POSITION) {
	client->active_position_x += offset;
      }

      /* Reset. */
      client->mode = NORMAL;
    }
    break;
  case CUB:
    {
      /* Parse the parameters. */
      unsigned short offset = 1;
      if (client->parameter_count >= 1) {
	offset = client->parameter[0];
      }
      /* Set the position if in bounds. */
      if (offset <= client->active_position_x) {
	client->active_position_x -= offset;
      }

      /* Reset. */
      client->mode = NORMAL;
    }
    break;
  case CUP:
    {
      /* Parse the parameters. */
      unsigned short new_y = 1;
      unsigned short new_x = 1;
      if (client->parameter_count >= 1) {
	new_y = client->parameter[0];
      }
      if (client->parameter_count >= 2) {
	new_x = client->parameter[1];
      }
      /* Set the position if in bounds. */
      if (new_y <= PAGE_LIMIT_POSITION &&
	  new_x <= LINE_LIMIT_POSITION) {
	client->active_position_y = new_y - 1;
	client->active_position_x = new_x - 1;
      }

      /* Reset. */
      client->mode = NORMAL;
    }
    break;
  case ED:
    {
      /* Parse the parameters. */
      unsigned short choice = 0;
      if (client->parameter_count >= 1) {
	choice = client->parameter[0];
      }
      switch (choice) {
      case ERASE_TO_PAGE_LIMIT:
	/* Erase the current line. */
	for (unsigned short x = client->active_position_x; x != LINE_LIMIT_POSITION; ++x) {
	  client->screen_buffer[client->active_position_y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
	}
	/* Erase the subsequent lines. */
	for (unsigned short y = client->active_position_y + 1; y != PAGE_LIMIT_POSITION; ++y) {
	  for (unsigned short x = 0; x != LINE_LIMIT_POSITION; ++x) {
	    client->screen_buffer[y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
	  }
	}
	break;
      case ERASE_TO_PAGE_HOME:
	/* Erase the preceding lines. */
	for (unsigned short y = 0; y != client->active_position_y; ++y) {
	  for (unsigned short x = 0; x != LINE_LIMIT_POSITION; ++x) {
	    client->screen_buffer[y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
	  }
	}
	/* Erase the current line. */
	for (unsigned short x = 0; x != client->active_position_x + 1; ++x) {
	  client->screen_buffer[client->active_position_y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
	}
	break;
      case ERASE_ALL:
	/* Erase the entire page. */
	for (unsigned short y = 0; y != PAGE_LIMIT_POSITION; ++y) {
	  for (unsigned short x = 0; x != LINE_LIMIT_POSITION; ++x) {
	    client->screen_buffer[y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
	  }
	}
	break;
      }

      client->mode = NORMAL;
    }
    break;
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

/*
  INPUT DEVICES -> TERMINAL
*/

/* scan_codes_in
   -------------
   Receive scan codes from the keyboard.
   
   Post: active client scan coodes buffer is not empty
 */
BEGIN_INPUT (NO_PARAMETER, SCAN_CODES_IN_NO, "scan_codes_in", "buffer_file_t", scan_codes_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) != 0) {
    end_input_action (bda, bdb);
  }
  
  size_t size = buffer_file_size (&input_buffer);
  const unsigned char* codes = buffer_file_readp (&input_buffer, size);
  if (codes == 0) {
    end_input_action (bda, bdb);
  }
  
  if (active_client != 0) {
    if (buffer_file_write (&active_client->scan_codes_buffer, codes, size) != 0) {
      syslog ("TODO:  Handle the error");
      end_input_action (bda, bdb);
    }
  }
  
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

/* mouse_packets_in
   ----------------
   Receive mouse packets from the mouse.
   
   Post: active client mouse packet list is not empty
 */
BEGIN_INPUT (NO_PARAMETER, MOUSE_PACKETS_IN_NO, "mouse_packets_in", "mouse_packet_list_t", mouse_packets_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (active_client != 0) {
    size_t count = 0;
    mouse_packet_t mouse_packet;
    mouse_packet_list_t mouse_packet_list;
    if (mouse_packet_list_initr (&mouse_packet_list, bda, &count) != 0) {
      end_input_action (bda, bdb);
    }
    
    for (size_t idx = 0; idx != count; ++idx) {
      if (mouse_packet_list_read (&mouse_packet_list, &mouse_packet) != 0) {
	end_input_action (bda, bdb);
      }
      
      if (mouse_packet_list_write (&active_client->mouse_packet_list, &mouse_packet) != 0) {
	syslog ("TODO:  Handle the error.");
	end_input_action (bda, bdb);
      }
    }
  }

  end_input_action (bda, bdb);
}

/*
  TERMINAL -> CLIENT
*/

BEGIN_OUTPUT (AUTO_PARAMETER, SCAN_CODES_OUT_NO, "scan_codes_out", "buffer_file_t", scan_codes_out, ano_t ano, aid_t aid)
{
  initialize ();
  scheduler_remove (ano, aid);

  /* Find the client. */
  client_t* client = find_client (aid);
  if (client == 0) {
    /* The client does not exist.  Create it. */
    client = create_client (aid);
  }

  if (buffer_file_size (&client->scan_codes_buffer) != 0) {
    buffer_file_truncate (&client->scan_codes_buffer);
    end_output_action (true, client->scan_codes_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

BEGIN_OUTPUT (AUTO_PARAMETER, STDOUT_NO, "stdout", "buffer_file_t", stdout, ano_t ano, aid_t aid)
{
  initialize ();
  scheduler_remove (ano, aid);

  /* Find the client. */
  client_t* client = find_client (aid);
  if (client == 0) {
    /* The client does not exist.  Create it. */
    client = create_client (aid);
  }

  if (buffer_file_size (&client->ascii_buffer) != 0) {
    buffer_file_truncate (&client->ascii_buffer);
    end_output_action (true, client->ascii_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

BEGIN_OUTPUT (AUTO_PARAMETER, MOUSE_PACKETS_OUT_NO, "mouse_packets_out", "mouse_packet_list_t", mouse_packets_out, ano_t ano, aid_t aid)
{
  initialize ();
  scheduler_remove (ano, aid);

  /* Find the client. */
  client_t* client = find_client (aid);
  if (client == 0) {
    /* The client does not exist.  Create it. */
    client = create_client (aid);
  }

  if (client->mouse_packet_list.count != 0) {
    if (mouse_packet_list_reset (&client->mouse_packet_list) != 0) {
      syslog ("TODO:  Handle error.");
      end_output_action (false, -1, -1);
    }
    end_output_action (true, client->mouse_packets_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

/*
  CLIENT -> TERMINAL
*/

BEGIN_INPUT (AUTO_PARAMETER, STDIN_NO, "stdin", "buffer_file_t", stdin, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) != 0) {
    end_input_action (bda, bdb);
  }

  size_t size = buffer_file_size (&input_buffer);
  const char* begin = buffer_file_readp (&input_buffer, size);
  if (begin == 0) {
    end_input_action (bda, bdb);
  }


  /* Find or create the client. */
  client_t* client = find_client (aid);
  if (client == 0) {
    client = create_client (aid);
  }

  size_t old_cursor_location = 0;
  if (client == active_client) {
    old_cursor_location = active_client->active_position_y * LINE_LIMIT_POSITION + active_client->active_position_x;
  }

  /* Process the string. */
  const char* end = begin + size;
  for (; begin != end; ++begin) {
    const char c = *begin;
    if ((c & 0x80) == 0) {
      switch (client->mode) {
      case NORMAL:
  	process_normal (client, c);
  	break;
      case ESCAPED:
  	process_escaped (client, c);
  	break;
      case CONTROL:
  	process_control (client, c);
  	break;
      }
    }
  }

  if (client == active_client) {
    /* TODO:  Check that this is necessary. */
    /* Send the data. */
    if (vga_op_list_write_bassign (&vga_op_list, VGA_TEXT_MEMORY_BEGIN, PAGE_LIMIT_POSITION * LINE_LIMIT_POSITION * CELL_SIZE, active_client->screen_bd) != 0) {
      syslog ("terminal: Could not write vga op list");
      exit ();
    }

    size_t new_cursor_location = active_client->active_position_y * LINE_LIMIT_POSITION + active_client->active_position_x;
    if (new_cursor_location != old_cursor_location) {
      /* Send the cursor. */
      if (vga_op_list_write_set_cursor_location (&vga_op_list, new_cursor_location) != 0) {
      	syslog ("terminal: Could not write vga op list");
      	exit ();
      }
    }
  }
  

  end_input_action (bda, bdb);
}

/*
  TERMINAL -> OUTPUT DEVICES
*/

/* vga_op
   ------
   Send commands to the vga.
   
   Pre:  vga_op_list is not empty
   Post: vga_op_list is empty
 */
static bool
vga_op_precondition (void)
{
  return vga_op_list.count != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, VGA_OP_NO, "vga_op", "vga_op_list", vga_op, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (VGA_OP_NO, param);

  if (vga_op_precondition ()) {
    vga_op_list_reset (&vga_op_list);
    end_output_action (true, vga_op_list_bda, vga_op_list_bdb);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

BEGIN_SYSTEM_INPUT (DESTROYED_NO, "", "", destroyed, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  /* Destroy the client. */
  client_t** ptr = &client_list_head;
  for (; *ptr != 0 && (*ptr)->aid != aid; ptr = &(*ptr)->next) ;;
  if (*ptr != 0) {
    client_t* temp = *ptr;
    *ptr = temp->next;
    destroy_client (temp);
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
  if (active_client != 0 && buffer_file_size (&active_client->scan_codes_buffer) != 0) {
    scheduler_add (SCAN_CODES_OUT_NO, active_client->aid);
  }
  if (active_client != 0 && buffer_file_size (&active_client->ascii_buffer) != 0) {
    scheduler_add (STDOUT_NO, active_client->aid);
  }
  if (active_client != 0 && active_client->mouse_packet_list.count != 0) {
    scheduler_add (MOUSE_PACKETS_OUT_NO, active_client->aid);
  }
  if (vga_op_precondition ()) {
    scheduler_add (VGA_OP_NO, 0);
  }
}
