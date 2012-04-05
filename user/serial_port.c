#include <automaton.h>
#include <io.h>
#include <buffer_file.h>
#include <string.h>
#include <description.h>
#include <dymem.h>
#include "syslog.h"
 
/*
  Serial Port Driver
  ==================
 */

#define INIT_NO 1
#define HALT_NO 2
#define SYSLOG_NO 3
#define INTERRUPT_NO 4
#define TEXT_OUT_NO 5
#define START_NO 6
#define STOP_NO 7
#define TEXT_IN_NO 8

#define INFO "serial_port: info: "
#define WARNING "serial_port: warning: "
#define ERROR "serial_port: error: "

#define PORT_BASE 0x3F8
#define IRQ 4

#define DEFAULT_BAUD 9600

/* Clock frequency of the UART in Hz. */
#define FREQ 115200

/* Registers when Divisor Latch Bit (DLAB) = 0. */
#define RECEIVE_BUFFER_REG 0
#define TRANSMIT_BUFFER_REG 0
#define INTERRUPT_ENABLE_REG 1
#define INTERRUPT_ID_REG 2
#define FIFO_CONTROL_REG 2
#define LINE_CONTROL_REG 3
#define MODEM_CONTROL_REG 4
#define LINE_STATUS_REG 5
#define MODEM_STATUS_REG 6
#define SCRATCH_REG 7

/* Registers when DLAB = 1. */
#define DIVISOR_LSB_REG 0
#define DIVISOR_MSB_REG 1
#define ALT_FUNC_REG 2

#define REGISTER_COUNT 8

/* Bits in INTERRUPT_ENABLE_REG. */
#define ENABLE_RECEIVED_DATA_AVAILABLE_INT (1 << 0)
#define ENABLE_TRANSMIT_HOLDING_REGISTER_EMPTY_INT (1 << 1)
#define ENABLE_RECEIVER_LINE_STATUS_INT (1 << 2)
#define ENABLE_MODEM_STATUS_INT (1 << 3)

/* Bits in the INTERRUPT_ID_REG. */
#define NO_INTERRUPT (1 << 0)
#define MODEM_STATUS_CHANGED_INT (0 << 1)
#define TRANSMIT_HOLDING_REGISTER_EMPTY_INT (1 << 1)
#define RECEIVED_DATA_AVAILABLE_INT (2 << 1)
#define RECEIVER_LINE_STATUS_INT (3 << 1)
#define TRANSMIT_MACHINE_INT (4 << 1)
#define TIMER_INTERRUPT_INT (5 << 1)
#define CHARACTER_TIMEOUT_INT (6 << 1)
#define INTERRUPT_MASK (7 << 1)

/* Bits in LINE_CONTROL_REG. */
#define BITS_5 (0 << 0)
#define BITS_6 (1 << 0)
#define BITS_7 (2 << 0)
#define BITS_8 (3 << 0)
#define STOP_1 (0 << 2)
#define STOP_2 (1 << 2)
#define NO_PARITY (0 << 3)
#define ODD_PARITY (1 << 3)
#define EVEN_PARITY (3 << 3)
#define ZERO_PARITY (5 << 3)
#define ONE_PARITY (7 << 3)
#define SEND_BREAK (1 << 6)
#define DLAB (1 << 7)

/* Bits in FIFO_CONTROL_REG. */
#define FIFO_ENABLE (1 << 0)
#define RECEIVE_FIFO_RESET (1 << 1)
#define TRANSMIT_FIFO_RESET (1 << 2)
#define DMA_MODE_SELECT (1 << 3)
#define LEVEL_1 (0 << 6)
#define LEVEL_4 (1 << 6)
#define LEVEL_8 (2 << 6)
#define LEVEL_14 (3 << 6)

/* Bits in the MODEM_CONTROL_REG. */
/* Data terminal ready (ready to receive data). */
#define DTR (1 << 0)
/* Request to send (ready to send data). */
#define RTS (1 << 1)
#define IRQ_ENABLE (1 << 3)

/* Bits in the LINE_STATUS_REG. */
#define RECEIVED_DATA_READY (1 << 0)
#define OVERRUN_ERROR (1 << 1)
#define PARITY_ERROR (1 << 2)
#define FRAMING_ERROR (1 << 3)
#define BREAK_INTERRUPT (1 << 4)
#define TRANSMIT_HOLDING_REGISTER_EMPTY (1 << 5)
#define TRANSMIT_SHIFT_REGISTER_EMPTY (1 << 6)
#define RECEIVE_FIFO_ERROR (1 << 7)

/* Initialization flag. */
static bool initialized = false;

typedef enum {
  RUN,
  HALT,
} state_t;
static state_t state = RUN;

/* Syslog buffer. */
static bd_t syslog_bd = -1;
static buffer_file_t syslog_buffer;

/* Standard output buffer. */
static bd_t text_out_bd = -1;
static buffer_file_t text_out_buffer;

static unsigned short
baud_to_divisor (unsigned short baud)
{
  unsigned short divisor = FREQ / baud;
  if (divisor == 0) {
    divisor = 1;
  }
  return divisor;
}

static bool
transmit_holding_register_empty (void)
{
  return inb (PORT_BASE + LINE_STATUS_REG) & TRANSMIT_HOLDING_REGISTER_EMPTY;
}

static bool
received_data_ready (void)
{
  return inb (PORT_BASE + LINE_STATUS_REG) & RECEIVED_DATA_READY;
}

typedef struct buffer buffer_t;
struct buffer {
  unsigned char* data;
  size_t size;
  size_t pos;
  buffer_t* next;
};

static buffer_t* buffer_head;
static buffer_t** buffer_tail = &buffer_head;

static void
send (void)
{
  /* We assume FIFO mode where we can send 16 bytes. */
  for (size_t count = 0; count != 16 && buffer_head != 0; ++count) {
    outb (PORT_BASE + TRANSMIT_BUFFER_REG, buffer_head->data[buffer_head->pos++]);
    if (buffer_head->pos == buffer_head->size) {
      buffer_t* buffer = buffer_head;
      buffer_head = buffer->next;
      if (buffer_head == 0) {
	buffer_tail = &buffer_head;
      }
      
      free (buffer->data);
      free (buffer);
    }
  }
}

static void
push_buffer (const void* data,
	     size_t size)
{
  buffer_t* buffer = malloc (sizeof (buffer_t));
  memset (buffer, 0, sizeof (buffer_t));
  buffer->data = malloc (size);
  memcpy (buffer->data, data, size);
  buffer->size = size;
  
  *buffer_tail = buffer;
  buffer_tail = &buffer->next;
  
  if (buffer == buffer_head && transmit_holding_register_empty ()) {
    send ();
  }
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

    aid_t syslog_aid = lookup (SYSLOG_NAME, strlen (SYSLOG_NAME) + 1);
    if (syslog_aid != -1) {
      /* Bind to the syslog. */

      description_t syslog_description;
      if (description_init (&syslog_description, syslog_aid) != 0) {
	exit ();
      }
      
      action_desc_t syslog_text_in;
      if (description_read_name (&syslog_description, &syslog_text_in, SYSLOG_TEXT_IN) != 0) {
	exit ();
      }
      
      /* We bind the response first so they don't get lost. */
      if (bind (getaid (), SYSLOG_NO, 0, syslog_aid, syslog_text_in.number, 0) == -1) {
	exit ();
      }

      description_fini (&syslog_description);
    }

    text_out_bd = buffer_create (0);
    if (text_out_bd == -1) {
      bfprintf (&syslog_buffer, ERROR "could not create output buffer\n");
      state = HALT;
      return;
    }
    if (buffer_file_initw (&text_out_buffer, text_out_bd) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not initialize output buffer\n");
      state = HALT;
      return;
    }

    /* Reserve the ports. */
    for (unsigned short idx = 0; idx != REGISTER_COUNT; ++idx) {
      if (reserve_port (PORT_BASE + idx) != 0) {
	bfprintf (&syslog_buffer, ERROR "could not reserve I/O port %x\n", PORT_BASE + idx);
	state = HALT;
	return;
      }
    }

    /* Subscribe to the IRQ. */
    if (subscribe_irq (IRQ, INTERRUPT_NO, 0) != 0) {
      bfprintf (&syslog_buffer, ERROR "could not subscribe to IRQ %d\n", IRQ);
      state = HALT;
      return;
    }

    /* Initialize the serial port. */
    
    /* Enable the receive interrupt. */
    outb (PORT_BASE + INTERRUPT_ENABLE_REG, ENABLE_RECEIVED_DATA_AVAILABLE_INT | ENABLE_TRANSMIT_HOLDING_REGISTER_EMPTY_INT);
    /* Set the divisor (baud).  This wipes out all other settings. */
    outb (PORT_BASE + LINE_CONTROL_REG, DLAB);
    unsigned short divisor = baud_to_divisor (DEFAULT_BAUD);    
    outb (PORT_BASE + DIVISOR_LSB_REG, divisor & 0xFF);
    outb (PORT_BASE + DIVISOR_MSB_REG, (divisor >> 8) & 0xFF);
    /* Set the frame type. */
    outb (PORT_BASE + LINE_CONTROL_REG, BITS_8 | NO_PARITY | STOP_1);
    /* Reset the FIFO. */
    outb (PORT_BASE + FIFO_CONTROL_REG, LEVEL_14 | TRANSMIT_FIFO_RESET | RECEIVE_FIFO_RESET | FIFO_ENABLE);
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

/* Halt
   ----
   Halt the automaton.
   
   Pre:  state == HALT and syslog_buffer is empty
   Post: 
*/
static bool
halt_precondition (void)
{
  return state == HALT && buffer_file_size (&syslog_buffer) == 0;
}

BEGIN_INTERNAL (NO_PARAMETER, HALT_NO, "", "", halt, ano_t ano, int param)
{
  initialize ();

  if (halt_precondition ()) {
    exit ();
  }
  finish_internal ();
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

  if (syslog_precondition ()) {
    buffer_file_truncate (&syslog_buffer);
    finish_output (true, syslog_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

BEGIN_SYSTEM_INPUT (INTERRUPT_NO, "", "", interrupt, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  unsigned char interrupt_type = inb (PORT_BASE + INTERRUPT_ID_REG);

  if ((interrupt_type & NO_INTERRUPT) == 0) {
    switch (interrupt_type & INTERRUPT_MASK) {
    case RECEIVED_DATA_AVAILABLE_INT:
    case CHARACTER_TIMEOUT_INT:
      while (received_data_ready ()) {
	if (buffer_file_put (&text_out_buffer, inb (PORT_BASE + RECEIVE_BUFFER_REG)) != 0) {
	  bfprintf (&syslog_buffer, ERROR "could not write to output buffer\n");
	  state = HALT;
	  finish_input (bda, bdb);
	}
      }
      break;
    case TRANSMIT_HOLDING_REGISTER_EMPTY_INT:
      send ();
      break;
    case MODEM_STATUS_CHANGED_INT:
    case RECEIVER_LINE_STATUS_INT:
    case TRANSMIT_MACHINE_INT:
    case TIMER_INTERRUPT_INT:
      bfprintf (&syslog_buffer, WARNING "unknown interrupt type %x\n", interrupt_type & INTERRUPT_MASK);
      break;
    }
  }

  finish_input (bda, bdb);
}

static bool
text_out_precondition (void)
{
  return buffer_file_size (&text_out_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, TEXT_OUT_NO, "text_out", "buffer_file_t", text_out, ano_t ano, int param)
{
  initialize ();

  if (text_out_precondition ()) {
    buffer_file_truncate (&text_out_buffer);
    finish_output (true, text_out_bd, -1);
  }

  finish_output (false, -1, -1);
}

BEGIN_INPUT (NO_PARAMETER, START_NO, "start", "", start, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  /* Enable TX/RX. */
  outb (PORT_BASE + MODEM_CONTROL_REG, IRQ_ENABLE | RTS | DTR);

  finish_input (bda, bdb);
}

BEGIN_INPUT (NO_PARAMETER, STOP_NO, "stop", "", stop, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  /* Disable TX/RX. */
  outb (PORT_BASE + MODEM_CONTROL_REG, 0);

  finish_input (bda, bdb);
}

BEGIN_INPUT (NO_PARAMETER, TEXT_IN_NO, "text_in", "buffer_file_t", text_in, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) != 0) {
    bfprintf (&syslog_buffer, WARNING "bad input buffer\n");
    finish_input (bda, bdb);
  }

  const size_t size = buffer_file_size (&input_buffer);
  if (size != 0) {
    const unsigned char* data = buffer_file_readp (&input_buffer, size);
    if (data == 0) {
      bfprintf (&syslog_buffer, WARNING "bad input buffer\n");
      finish_input (bda, bdb);
    }

    /* Enqueue the buffer.
       TODO:  If the buffer is really big, then we should retain a reference to bda instead of copying.
    */
    push_buffer (data, size);
  }

  finish_input (bda, bdb);
}

void
do_schedule (void)
{
  if (halt_precondition ()) {
    schedule (HALT_NO, 0);
  }
  if (syslog_precondition ()) {
    schedule (SYSLOG_NO, 0);
  }
  if (text_out_precondition ()) {
    schedule (TEXT_OUT_NO, 0);
  }
}
