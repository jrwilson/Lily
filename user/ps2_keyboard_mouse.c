#include <automaton.h>
#include <io.h>
#include <fifo_scheduler.h>
#include <string.h>
#include <buffer_file.h>
#include <description.h>
#include "mouse_msg.h"
#include "syslog.h"

/*
  PS2 Keyboard and Mouse Driver
  =============================

  The keyboard and mouse system is organized according to the following diagram:

  CPU <=> Controller <-> Keyboard / Mouse

  The peripheral controller communicates with the keyboard and mouse via a serial link.
  The peripheral controller communicates with the CPU using two I/O ports and interrupts.
  Reading the CONTROLLER_PORT returns a byte describing the status of the controller.
  Writing a byte to the CONTROLLER_PORT causes the controller to perform some action.
  Reading or writing the DATA_PORT causes interactions with the controller or keyboard/mouse.
  It is very important to understand that all interactions with the keyboard/mouse are mediated by the controller.
  Thus, to interact with the mouse, one must first configure the controller to communicate with the mouse and then configure the mouse.

  The keyboard is implemented as a system input for receiving the keyboard interrupt and an output action to relay the scan codes.
  The system input reads the scanode from the DATA_PORT and appends it to a buffer that is used in the output action.

  The mouse is implemented in a similar fashion using a system input to process the interrupt and an output action to relay mouse packets.
  The mouse implementation is more complex as a mouse packet consists of multiple bytes where each byte is deliver via an interrupt.

  Currently, the following mice are supported:
  Standard PS/2 mouse (3 buttons)
  IntelliMouse (3 buttons and scroll wheel)
  Five button (5 buttons and scroll wheel)
*/

/* Data read/written on this port passes through the controller to the keyboard/mouse. */
#define DATA_PORT 0x60
/* Data read/written on this port pertains to the controller. */
#define CONTROLLER_PORT 0x64

/* Command to get port status */
#define CONTROLLER_GET_COMPAQ_STATUS_BYTE 0x20
/* Command to set port status */
#define CONTROLLER_SET_COMPAQ_STATUS_BYTE 0x60
/* Command to enable auxiliary device */
#define CONTROLLER_ENABLE_AUX_DEVICE 0xA8
/* Command to handshake with mouse data port before sending */
#define CONTROLLER_WRITE_TO_MOUSE 0xD4

/* Command to get mouse id  */
#define MOUSE_GET_ID 0xF2
/* Command to get mouse id  */
#define MOUSE_SET_SAMPLE_RATE 0xF3
/* Command to enable transmission of mouse packets */
#define MOUSE_ENABLE_MOVEMENT_PACKETS 0xF4
/* Command to set mouse defaults */
#define MOUSE_SET_DEFAULTS_AND_DISABLE 0xF6

/* Acknowledgement byte from mouse */
#define MOUSE_COMMAND_ACK 0xFA
/* Error response byte from mouse */
#define MOUSE_COMMAND_ERROR 0xFC
/* Negative acknowledgement byte from mouse */
#define MOUSE_COMMAND_NACK 0xFE

/* Compaq status bits */
#define ENABLE_IRQ12_BIT 0x2 /* Bit to set to enable aux to generate IRQ12 */
#define DISABLE_MOUSE_CLOCK_BIT 0x20 /* Bit to clear to disable mouse clock */

/* Controller buffer state bits */
#define CONTROLLER_WRITE_BUFFER_FULL_BIT 0x02 /* Bit that indicates write buffer is full */
#define CONTROLLER_READ_READY_BIT 0x01  /* Bit that is set when data port is ready for a read */

/* Mouse packet bits */
#define BAD_MOUSE_PACKET_BITS (0x80 | 0x40)
#define BASIC_MOUSE_BUTTON_STATUS_BITS (0x4 | 0x2 | 0x1)
#define MOUSE_DELTA_X_SIGN_BIT 0x10 
#define MOUSE_DELTA_Y_SIGN_BIT 0x20
#define EXTRA_BUTTON_STATUS_BITS (0x20 | 0x10)
#define BONUS_BYTE_Z_BITS (0x8 | 0x4 | 0x2 | 0x1)

#define EXTRA_BUTTON_STATUS_OFFSET 1

/* Valid mouse packet z movement values */
#define MOUSE_Z_ZERO 0x0
#define MOUSE_Z_UP 0x1
#define MOUSE_Z_DOWN 0xF
#define MOUSE_Z_LEFT 0xE
#define MOUSE_Z_RIGHT 0x2

/* These are the IRQs generated by the controller. */
#define KEYBOARD_IRQ 1
#define MOUSE_IRQ 12

/* Initialization flag. */
static bool initialized = false;

#define ERROR "ps2_keyboard_mouse: error: "
#define INFO "ps2_keyboard_mouse: info: "


typedef enum {
  RUN,
  STOP,
} state_t;
static state_t state = RUN;

/* Output buffer for syslog. */
static bd_t syslog_bd = -1;
static buffer_file_t syslog_buffer;

/* Output buffer for keyboard scan codes. */
static bd_t scan_codes_bd = -1;
static buffer_file_t scan_codes_buffer;

/* Output buffer for mouse packets. */
static bd_t mouse_packets_bd = -1;
static mouse_packet_list_t mouse_packet_list;

/* Mouse state machine. */
typedef enum {
  BASIC_MOUSE = 0,
  SCROLL_WHEEL_MOUSE = 3,
  FIVE_BUTTON_SCROLL_WHEEL_MOUSE = 4,
} mouse_id_t;
static mouse_id_t mouse_id = BASIC_MOUSE;
typedef enum {
  STATUS_BYTE = 0,
  DELTA_X_BYTE = 1,
  DELTA_Y_BYTE = 2,
  BONUS_BYTE = 3,
  MAX_MOUSE_PACKET_SIZE
} mouse_byte_t;
static mouse_byte_t mouse_byte = STATUS_BYTE;
static unsigned char mouse_packet_data [MAX_MOUSE_PACKET_SIZE];

#define INIT_NO 1
#define STOP_NO 2
#define SYSLOG_NO 3
#define KEYBOARD_INTERRUPT_NO 4
#define SCAN_CODES_NO 5
#define MOUSE_INTERRUPT_NO 6
#define MOUSE_PACKETS_OUT_NO 7

/* Basic I/O routines. */
static unsigned char
controller_status (void)
{
  return inb (CONTROLLER_PORT);
}

static void
wait_to_send ()
{
  while (controller_status () & CONTROLLER_WRITE_BUFFER_FULL_BIT) ;;
}

static unsigned char
read_byte ()
{
  while ((controller_status () & CONTROLLER_READ_READY_BIT) == 0) ;;
  return inb (DATA_PORT);
}

static void
controller_command (unsigned char c)
{
  wait_to_send ();
  outb (CONTROLLER_PORT, c);
}

static void
mouse_write (unsigned char c)
{
  /* First, tell the controller we are sending data to the mouse. */
  controller_command (CONTROLLER_WRITE_TO_MOUSE);
  /* Then, send the data. */
  wait_to_send ();
  outb (DATA_PORT, c);
}

/* Controller routines. */
static void
controller_enable_aux_device (void)
{
  controller_command (CONTROLLER_ENABLE_AUX_DEVICE);
}

static unsigned char
controller_get_compaq_status_byte (void)
{
  controller_command (CONTROLLER_GET_COMPAQ_STATUS_BYTE);
  return read_byte ();
}

static void
controller_set_compaq_status_byte (unsigned char status)
{
  controller_command (CONTROLLER_SET_COMPAQ_STATUS_BYTE);
  wait_to_send ();
  outb (DATA_PORT, status);
}

/* Mouse routines. */
static int
mouse_set_sample_rate (unsigned char sample_rate)
{
  mouse_write (MOUSE_SET_SAMPLE_RATE);
  unsigned char c = read_byte ();
  if (c != MOUSE_COMMAND_ACK) {
    bfprintf (&syslog_buffer, ERROR "mouse responded to set sample rate with %x\n", c);
    return -1;
  }
  mouse_write (sample_rate);
  c = read_byte ();
  if (c != MOUSE_COMMAND_ACK) {
    bfprintf (&syslog_buffer, ERROR "mouse responded to sample rate with %x\n", c);
    return -1;
  }
  return 0;
}

static int
mouse_get_id (mouse_id_t* id)
{
  mouse_write (MOUSE_GET_ID);
  unsigned char c = read_byte ();
  if (c != MOUSE_COMMAND_ACK) {
    bfprintf (&syslog_buffer, ERROR "mouse responded to get id with %x\n", c);
    return -1;
  }
  *id = read_byte ();
  return 0;
}

static int
mouse_set_defaults_and_disable (void)
{
  mouse_write (MOUSE_SET_DEFAULTS_AND_DISABLE);
  unsigned char c = read_byte ();
  if (c != MOUSE_COMMAND_ACK) {
    bfprintf (&syslog_buffer, ERROR "mouse responded to set defaults and disable with %x\n", c);
    return -1;
  }
  return 0;
}

static int
mouse_enable_movement_packets (void)
{
  mouse_write (MOUSE_ENABLE_MOVEMENT_PACKETS);
  unsigned char c = read_byte ();
  if (c != MOUSE_COMMAND_ACK) {
    bfprintf (&syslog_buffer, ERROR "mouse responded to enable movement packets with %x\n", c);
    return -1;
  }
  return 0;
}

static int
mouse_send_scroll_wheel_sequence () {
  if (mouse_id == BASIC_MOUSE) {
    if (mouse_set_sample_rate (200) != 0 ||
	mouse_set_sample_rate (100) != 0 ||
	mouse_set_sample_rate (80) != 0 ||
	mouse_get_id (&mouse_id) != 0) {
      return -1;
    }
  }
  
  return 0;
}

static int
mouse_send_five_button_sequence () {
  if (mouse_id == SCROLL_WHEEL_MOUSE) {
    if (mouse_set_sample_rate (200) != 0 ||
	mouse_set_sample_rate (200) != 0 ||
	mouse_set_sample_rate (80) != 0 ||
	mouse_get_id (&mouse_id) != 0) {
      return -1;
    }
  }

  return 0;
}

static void
write_mouse_packet () {
  /* do nothing if either bad packet bit is set */
  if ((mouse_packet_data[STATUS_BYTE] & BAD_MOUSE_PACKET_BITS) == 0) {

    mouse_packet_t packet;
    packet.button_status_bits = mouse_packet_data[STATUS_BYTE] & BASIC_MOUSE_BUTTON_STATUS_BITS;
    packet.x_delta = mouse_packet_data[DELTA_X_BYTE];
    packet.y_delta = mouse_packet_data[DELTA_Y_BYTE];
    packet.z_delta_vertical = 0;
    packet.z_delta_horizontal = 0;

    if (getmonotime (&packet.time_stamp) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not get the time\n");
      state = STOP;
      return;
    }

    switch (mouse_id) {

    case FIVE_BUTTON_SCROLL_WHEEL_MOUSE:
      packet.button_status_bits |= (mouse_packet_data[BONUS_BYTE] & EXTRA_BUTTON_STATUS_BITS) >> EXTRA_BUTTON_STATUS_OFFSET;

      /* This all assumes 1 dimensional track ball scrolling per message */
    case SCROLL_WHEEL_MOUSE:
      switch (mouse_packet_data[BONUS_BYTE] & BONUS_BYTE_Z_BITS) {
      case MOUSE_Z_UP:
	packet.z_delta_vertical = 1;
	break;
      case MOUSE_Z_DOWN:
	packet.z_delta_vertical = -1;
	break;
      case MOUSE_Z_LEFT:
	packet.z_delta_horizontal = -1;
	break;
      case MOUSE_Z_RIGHT:
	packet.z_delta_horizontal = 1;
	break;
      case MOUSE_Z_ZERO:
	break;
      default:
	bfprintf (&syslog_buffer, INFO "unexpected mouse z movement value %x\n", mouse_packet_data[BONUS_BYTE] & BONUS_BYTE_Z_BITS);
	break;
      }

    default: // BASIC_MOUSE
      /* Extend the sign bytes. */
      if (mouse_packet_data[STATUS_BYTE] & MOUSE_DELTA_X_SIGN_BIT) {
	packet.x_delta |= 0xFFFFFF00;
      }
      if (mouse_packet_data[STATUS_BYTE] & MOUSE_DELTA_Y_SIGN_BIT) {
	packet.y_delta |= 0xFFFFFF00;
      }
    }

    if (mouse_packet_list_write (&mouse_packet_list, &packet) != 0) {
      bfprintf (&syslog_buffer, ERROR "failed to write to mouse packet list\n");
      state = STOP;
      return;
    }
  }
}

/* Initialization that must succeed. */
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

    /* Create the keyboard output buffer. */
    scan_codes_bd = buffer_create (0);
    if (scan_codes_bd == -1) {
      bfprintf (&syslog_buffer, ERROR "could not create scan codes buffer\n");
      state = STOP;
      return;
    }
    if (buffer_file_initw (&scan_codes_buffer, scan_codes_bd) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not initialize scan codes buffer\n");
      state = STOP;
      return;
    }

    /* Create the mouse output buffer. */
    mouse_packets_bd = buffer_create (0);
    if (mouse_packets_bd == -1) {
      bfprintf (&syslog_buffer, ERROR "could not create mouse packets buffer\n");
      state = STOP;
      return;
    }
    if (mouse_packet_list_initw (&mouse_packet_list, mouse_packets_bd) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not initialize mouse packets buffer\n");
      state = STOP;
      return;
    }

    /* Reserve ports. */
    if (reserve_port (DATA_PORT) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not reserve data port\n");
      state = STOP;
      return;
    }
    if (reserve_port (CONTROLLER_PORT) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not reserve controller port\n");
      state = STOP;
      return;
    }
    
    /* Reserve IRQs. */
    if (subscribe_irq (KEYBOARD_IRQ, KEYBOARD_INTERRUPT_NO, 0) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not subscribe to keyboard irq\n");
      state = STOP;
      return;
    }
    
    if (subscribe_irq (MOUSE_IRQ, MOUSE_INTERRUPT_NO, 0) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not subscribe to mouse irq\n");
      state = STOP;
      return;
    }
    
    /* Handshakes with mouse hardware to initialize, done exactly once. */
    
    controller_enable_aux_device ();

    unsigned char c = controller_get_compaq_status_byte ();
    c |= ENABLE_IRQ12_BIT;
    c &= ~DISABLE_MOUSE_CLOCK_BIT;
    controller_set_compaq_status_byte (c);

    if (mouse_set_defaults_and_disable () != 0) {
      state = STOP;
      return;
    }

    /* Try to enable the scroll wheel and buttons 4 and 5. */
    if (mouse_send_scroll_wheel_sequence () != 0 ||
	mouse_send_five_button_sequence () != 0) {
      state = STOP;
      return;
    }
    
    switch (mouse_id) {
    case BASIC_MOUSE:
    case SCROLL_WHEEL_MOUSE:
    case FIVE_BUTTON_SCROLL_WHEEL_MOUSE:
      break;
    default:
      bfprintf (&syslog_buffer, ERROR "unknown mouse id %x\n", mouse_id);
      state = STOP;
      return;
    }
    
    if (mouse_enable_movement_packets () != 0) {
      state = STOP;
      return;
    }
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

/* keyboard_interrupt
   ------------------
   Process a keyboard interrupt by reading the scan code.
   
   Post: scan_codes_buffer is not empty
*/
BEGIN_SYSTEM_INPUT (KEYBOARD_INTERRUPT_NO, "", "", keyboard_interrupt, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  if (buffer_file_put (&scan_codes_buffer, read_byte ()) != 0) {
    bfprintf (&syslog_buffer, ERROR "could not write to output buffer\n");
    state = STOP;
  }
  end_input_action (bda, bdb);
}

/* scan_code
   ---------
   Output the scan codes.
   
   Pre:  scan_codes_buffer is not empty
   Post: scan_codes_buffer is empty
*/
static bool
scan_codes_precondition (void)
{
  return buffer_file_size (&scan_codes_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, SCAN_CODES_NO, "scan_codes", "buffer_file_t", scan_codes, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (ano, param);

  if (scan_codes_precondition ()) {
    buffer_file_truncate (&scan_codes_buffer);
    end_output_action (true, scan_codes_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

/* mouse_interrupt
   ---------------
   Process a mouse interrupt by reading a byte of the mouse packet.
   
   Post: advances the mouse state machine
*/
BEGIN_SYSTEM_INPUT (MOUSE_INTERRUPT_NO, "", "", mouse_interrupt, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  mouse_packet_data [mouse_byte] = read_byte ();

  switch (mouse_byte) {
  case STATUS_BYTE:
    mouse_byte = DELTA_X_BYTE;
    break;
  case DELTA_X_BYTE:
    mouse_byte = DELTA_Y_BYTE;
    break;
  case DELTA_Y_BYTE:
    if (mouse_id == BASIC_MOUSE) {
      mouse_byte = STATUS_BYTE;
      write_mouse_packet ();
    }
    else {
      mouse_byte = BONUS_BYTE;
    }
    break;
  case BONUS_BYTE:
    mouse_byte = STATUS_BYTE;
    write_mouse_packet ();
    break;
  default:
    bfprintf (&syslog_buffer, ERROR "unrecognized packet byte state\n");
    state = STOP;
    end_input_action (bda, bdb);
  }

  end_input_action (bda, bdb);
}

/* mouse_packets
   -------------
   Output the list of mouse packets.
   
   Pre:  mouse_packet_list is not empty
   Post: mouse_packet_list is empty
*/
static bool
mouse_packets_out_precondition (void)
{
  return mouse_packet_list.count != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, MOUSE_PACKETS_OUT_NO, "mouse_packets_out", "mouse_packet_list_t", mouse_packets_out, ano_t ano, int param)
{
  initialize ();
  scheduler_remove (ano, param);

  if (mouse_packets_out_precondition ()) {
    if (mouse_packet_list_reset (&mouse_packet_list) != 0) {
      bfprintf (&syslog_buffer, ERROR "mouse packet list reset failed\n");
      state = STOP;
      end_output_action (false, -1, -1);
    }
    end_output_action (true, mouse_packets_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
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
  if (scan_codes_precondition ()) {
    scheduler_add (SCAN_CODES_NO, 0);
  }
  if (mouse_packets_out_precondition ()) {
    scheduler_add (MOUSE_PACKETS_OUT_NO, 0);
  }
}
