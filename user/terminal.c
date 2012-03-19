#include "terminal.h"
#include <automaton.h>
#include <dymem.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <buffer_file.h>
#include "vga_msg.h"

/*
  VGA Terminal Server
  ===================
  
  This server implements a terminal on a video graphics array (VGA).
  The server interprets a stream of a data and control sequences and renders them to a VGA.
  The server is designed for multiple clients where each client manipulates a private virtual terminal.
  The server provides a mechanism for choosing which virtual terminal is displayed on the VGA.

  Interface
  ---------
  FOCUS - Determine which client is rendered to the VGA.
  DISPLAY - Receive data and control sequences to be rendered.
  VGA_OP - Send operations to control the VGA.

  The ECMA-48 Standard
  --------------------
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
  The server is not efficient and sends the entire screen when updating.

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

#define SCAN_CODES_NO 1
#define MOUSE_PACKETS_NO 2
#define DESTROYED_NO 3
#define TEXT_NO 4
#define VGA_OP_NO 5
#define INIT_NO 6

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

/* White on black for VGA. */
#define ATTRIBUTE 0x0F00

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
  /* Buffer descriptor for the data associated with this client. */
  bd_t bd;
  /* Same only mapped in. */
  unsigned short* buffer;
  /* Active position.  These are zero based. */
  unsigned short active_position_x;
  unsigned short active_position_y;

  /* Flag indicating that the graphics for the active_client needs to be sent to the VGA. */
  bool graphics_refresh;
  /* Flag indicating that the cursor for the active_client needs to be sent to the VGA. */
  bool cursor_refresh;

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

/* Initialization flag. */
static bool initialized = false;

/* Buffers for the output. */
static bd_t output_buffer_bda = -1;
static bd_t output_buffer_bdb = -1;
static vga_op_list_t output_buffer;

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
  client->bd = buffer_create (size_to_pages (PAGE_LIMIT_POSITION * LINE_LIMIT_POSITION * CELL_SIZE));
  if (client->bd == -1) {
    syslog ("terminal: Could not create client buffer");
    exit ();
  }
  client->buffer = buffer_map (client->bd);
  if (client->buffer == 0) {
    syslog ("terminal: Could not map client buffer");
    exit ();
  }
  /* ECMA-48 states that the initial state of the characters must be "erased."
     We use spaces. */
  unsigned short* data = client->buffer;
  for (size_t y = 0; y != PAGE_LIMIT_POSITION; ++y) {
    for (size_t x = 0; x != LINE_LIMIT_POSITION; ++x) {
      data[y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
    }
  }
  /* I don't recall ECMA-48 specifying the initial state of the active position but this seems reasonable. */
  client->active_position_x = LINE_HOME_POSITION;
  client->active_position_y = PAGE_HOME_POSITION;

  client->graphics_refresh = true;
  client->cursor_refresh = true;

  client->mode = NORMAL;

  client->next = client_list_head;
  client_list_head = client;

  if (subscribe_destroyed (aid, DESTROYED_NO) != 0) {
    syslog ("terminal: Could not subscribe");
    exit ();
  }

  return client;
}

static void
switch_to_client (client_t* client)
{
  if (client != active_client) {
    /* Only refresh if there is a client. */
    if (client != 0) {
      client->graphics_refresh = true;
      client->cursor_refresh = true;
    }
    active_client = client;
  }
}

static void
destroy_client (client_t* client)
{
  if (client == active_client) {
    /* Switch to the null client. */
    switch_to_client (0);
  }
  buffer_destroy (client->bd);
  free (client);
}

static void
scroll (client_t* client)
{
  /* Move the data up one line. */
  memmove (client->buffer, &client->buffer[LINE_LIMIT_POSITION], (PAGE_LIMIT_POSITION - 1) * LINE_LIMIT_POSITION * CELL_SIZE);
  /* Clear the last line. */
  for (size_t x = 0; x != LINE_LIMIT_POSITION; ++x) {
    client->buffer[(PAGE_LIMIT_POSITION - 1) * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
  }
  /* Change the active y. */
  --client->active_position_y;

  client->graphics_refresh = true;
  client->cursor_refresh = true;
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    output_buffer_bda = buffer_create (0);
    if (output_buffer_bda == -1) {
      syslog ("terminal: Could not create output buffer");
      exit ();
    }

    output_buffer_bdb = buffer_create (0);
    if (output_buffer_bdb == -1) {
      syslog ("terminal: Could not create output buffer");
      exit ();
    }

    if (vga_op_list_initw (&output_buffer, output_buffer_bda, output_buffer_bdb) != 0) {
      syslog ("terminal: Could not initialize vga op list");
      exit ();
    }

  }
}

/* typedef struct { */
/*   aid_t aid; */
/* } focus_arg_t; */

/* void */
/* terminal_focus (int param, */
/* 		bd_t bd, */
/* 		focus_arg_t* a, */
/* 		size_t buffer_size) */
/* { */
/*   if (a != 0 && buffer_size >= sizeof (focus_arg_t)) { */
/*     /\* We don't care if the client is null. *\/ */
/*     switch_to_client (find_client (a->aid)); */
/*   } */

/*   schedule (); */
/*   scheduler_finish (bd, FINISH_DESTROY); */
/* } */
/* EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, TERMINAL_FOCUS, terminal_focus); */

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
      client->cursor_refresh = true;
    }
    break;
  case HT:
    /* Horizontal tab. */
    client->active_position_x = (client->active_position_x + 8) & ~(8-1);
    client->cursor_refresh = true;
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
    client->cursor_refresh = true;
    break;
  case CR:
    /* Carriage return. */
    client->active_position_x = 0;
    client->cursor_refresh = true;
    break;
  case ESC:
    /* Begin of escaped sequence. */
    client->mode = ESCAPED;
    break;
  case DEL:
    /* Rub-out the character under the cursor. */
    client->buffer[client->active_position_y * LINE_LIMIT_POSITION + client->active_position_x] = ATTRIBUTE | ' ';
    client->graphics_refresh = true;
    break;
  default:
    /* ASCII character that can be displayed. */
    if (c >= ' ' && c <= '~') {
      client->buffer[client->active_position_y * LINE_LIMIT_POSITION + client->active_position_x++] = ATTRIBUTE | c;
      
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

      client->graphics_refresh = true;
      client->cursor_refresh = true;
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
	client->buffer[client->active_position_y * LINE_LIMIT_POSITION + dest_x] =
	  client->buffer[client->active_position_y * LINE_LIMIT_POSITION + dest_x - count];
      }
      /* Replace with spaces. */
      for (unsigned short dest_x = client->active_position_x; dest_x != LINE_LIMIT_POSITION && count != 0; ++dest_x, --count) {
	client->buffer[client->active_position_y * LINE_LIMIT_POSITION + dest_x] = ATTRIBUTE | ' ';
      }

      /* Move to line home. */
      client->active_position_x = LINE_HOME_POSITION;

      client->graphics_refresh = true;
      client->cursor_refresh = true;

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

      client->cursor_refresh = true;

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

      client->cursor_refresh = true;

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

      client->cursor_refresh = true;

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

      client->cursor_refresh = true;

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

      client->cursor_refresh = true;

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
	  client->buffer[client->active_position_y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
	}
	/* Erase the subsequent lines. */
	for (unsigned short y = client->active_position_y + 1; y != PAGE_LIMIT_POSITION; ++y) {
	  for (unsigned short x = 0; x != LINE_LIMIT_POSITION; ++x) {
	    client->buffer[y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
	  }
	}
	break;
      case ERASE_TO_PAGE_HOME:
	/* Erase the preceding lines. */
	for (unsigned short y = 0; y != client->active_position_y; ++y) {
	  for (unsigned short x = 0; x != LINE_LIMIT_POSITION; ++x) {
	    client->buffer[y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
	  }
	}
	/* Erase the current line. */
	for (unsigned short x = 0; x != client->active_position_x + 1; ++x) {
	  client->buffer[client->active_position_y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
	}
	break;
      case ERASE_ALL:
	/* Erase the entire page. */
	for (unsigned short y = 0; y != PAGE_LIMIT_POSITION; ++y) {
	  for (unsigned short x = 0; x != LINE_LIMIT_POSITION; ++x) {
	    client->buffer[y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
	  }
	}
	break;
      }

      client->graphics_refresh = true;
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

/* scan_codes
   ----------
   Receive scan codes from the keyboard.
   
   Post: ???
 */
BEGIN_INPUT (NO_PARAMETER, SCAN_CODES_NO, "scan_codes", "buffer_file", scan_code, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  syslog ("scan_codes");
  /* buffer_file_t input_buffer; */
  /* if (buffer_file_initr (&input_buffer, bda) != 0) { */
  /*   end_input_action (bda, bdb); */
  /* } */

  /* size_t size = buffer_file_size (&input_buffer); */
  /* const unsigned char* codes = buffer_file_readp (&input_buffer, size); */
  /* if (codes == 0) { */
  /*   end_input_action (bda, bdb); */
  /* } */

  /* for (size_t idx = 0; idx != size; ++idx) { */
  /*   if (consume != 0) { */
  /*     --consume; */
  /*     continue; */
  /*   } */

  /*   unsigned char scan = codes[idx]; */
  /*   if (scan < BREAK_MASK) { */
  /*     if (!escaped) { */
  /* 	process_key_code (scan_to_key[scan], true); */
  /*     } */
  /*     else { */
  /* 	escaped = false; */
  /* 	/\* Ignore fake shifts. *\/ */
  /* 	if (scan != SCAN_LSHIFT) { */
  /* 	  process_key_code (escaped_scan_to_key[scan], true); */
  /* 	} */
  /*     } */
  /*   } */
  /*   else if (scan == 0xE0) { */
  /*     escaped = true; */
  /*   } */
  /*   else if (scan == 0xE1) { */
  /*     process_key_code (KEY_PAUSE, true); */
  /*     process_key_code (KEY_PAUSE, false); */
  /*     consume = 5; */
  /*   } */
  /*   else { */
  /*     /\* Break. *\/ */
  /*     scan -= BREAK_MASK; */
  /*     if (!escaped) { */
  /* 	process_key_code (scan_to_key[scan], false); */
  /*     } */
  /*     else { */
  /* 	escaped = false; */
  /* 	/\* Ignore fake shifts. *\/ */
  /* 	if (scan != SCAN_LSHIFT) { */
  /* 	  process_key_code (escaped_scan_to_key[scan], false); */
  /* 	} */
  /*     } */
  /*   } */
  /* } */

  end_input_action (bda, bdb);
}

/* mouse_packets
   -------------
   Receive mouse packets from the mouse.
   
   Post: ???
 */
BEGIN_INPUT (NO_PARAMETER, MOUSE_PACKETS_NO, "mouse_packets", "mouse_packet_list_t", mouse_packets, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  syslog ("mouse_packets");

  end_input_action (bda, bdb);
}

/* BEGIN_INPUT (AUTO_PARAMETER, TEXT_NO, "text", "buffer_file", text, ano_t ano, aid_t aid, bd_t bda, bd_t bdb) */
/* { */
/*   initialize (); */

/*   buffer_file_t input_buffer; */
/*   if (buffer_file_initr (&input_buffer, bda) != 0) { */
/*     end_input_action (bda, bdb); */
/*   } */

/*   size_t size = buffer_file_size (&input_buffer); */
/*   const char* begin = buffer_file_readp (&input_buffer, size); */
/*   if (begin == 0) { */
/*     end_input_action (bda, bdb); */
/*   } */

/*   /\* Find or create the client. *\/ */
/*   client_t* client = find_client (aid); */
/*   if (client == 0) { */
/*     client = create_client (aid); */
/*   } */
/*   else { */
/*     /\* The client buffer is appended to the output buffer. */
/*        Thus, the reference count for the client buffer is at least 2 after an output. */
/*        If we write to the client buffer, it will be copied for copy-on-write. */
/*        By reinitializing the output buffer, we potentially drop the reference count to 1 and avoid copy-on-write. */
/*     *\/ */
/*     if (vga_op_list_reset (&output_buffer) != 0) { */
/*       syslog ("terminal: error: Could not reset vga_op_list"); */
/*       exit (); */
/*     } */
/*   } */

/*   /\* Process the string. *\/ */
/*   const char* end = begin + size; */
/*   for (; begin != end; ++begin) { */
/*     const char c = *begin; */
/*     if ((c & 0x80) == 0) { */
/*       switch (client->mode) { */
/*       case NORMAL: */
/* 	process_normal (client, c); */
/* 	break; */
/*       case ESCAPED: */
/* 	process_escaped (client, c); */
/* 	break; */
/*       case CONTROL: */
/* 	process_control (client, c); */
/* 	break; */
/*       } */
/*     } */
/*   } */

/*   /\* TODO:  Remove this line. *\/ */
/*   switch_to_client (client); */

/*   end_input_action (bda, bdb); */
/* } */

/* vga_op
   ------
   Send the screen for the active client.
   
   Pre:  there is an active client that needs to send an update
   Post: the flags are send and output_buffer_initialized == false
 */
static bool
vga_op_precondition (void)
{
  return true; //active_client != 0 && (active_client->graphics_refresh || active_client->cursor_refresh);
}

BEGIN_OUTPUT (NO_PARAMETER, VGA_OP_NO, "vga_op", "vga_op_list", vga_op, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (VGA_OP_NO, param);

  if (vga_op_precondition ()) {

    if (vga_op_list_reset (&output_buffer) != 0) {
      syslog ("terminal: error: Could not reset vga_op_list");
      exit ();
    }

    const char* str = "HHeelllloo  WWoorrlldd!!";

    if (vga_op_list_write_assign (&output_buffer, VGA_TEXT_MEMORY_BEGIN + 2, str, strlen (str)) != 0) {
      syslog ("terminal: Could not write vga op list");
      exit ();
    }
 
    if (vga_op_list_write_set_cursor_location (&output_buffer, 1) != 0) {
      syslog ("terminal: Could not write vga op list");
      exit ();
    }


    /* if (active_client->graphics_refresh) { */
    /*   /\* Send the data. *\/ */
    /*   if (vga_op_list_write_assign (&output_buffer, 0, PAGE_LIMIT_POSITION * LINE_LIMIT_POSITION * CELL_SIZE, active_client->bd) != 0) { */
    /*   	syslog ("terminal: Could not write vga op list"); */
    /*   	exit (); */
    /*   } */
    /*   active_client->graphics_refresh = false; */
    /* } */

    /* if (active_client->cursor_refresh) { */
    /*   /\* Send the cursor. *\/ */
    /*   if (vga_op_list_write_set_cursor_location (&output_buffer, active_client->active_position_y * LINE_LIMIT_POSITION + active_client->active_position_x) != 0) { */
    /*   	syslog ("terminal: Could not write vga op list"); */
    /*   	exit (); */
    /*   } */
    /*   active_client->cursor_refresh = false; */
    /* } */

    end_output_action (true, output_buffer_bda, output_buffer_bdb);
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
  if (vga_op_precondition ()) {
    scheduler_add (VGA_OP_NO, 0);
  }
}
