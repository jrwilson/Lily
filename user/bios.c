/*
  bios.c
  ======
  Load devices based on data reported in the BIOS Data Area (BDA).
*/

#include <automaton.h>
#include <buffer_file.h>
#include <string.h>
#include <description.h>
#include <callback_queue.h>
#include <dymem.h>
#include "vfs_msg.h"

/* Physical address of the BIOS data area. */
#define BDA_ADDRESS 0

#define SERIAL_PORT_PATH "/bin/serial_port"
static int com_irq[] = { 4, 3, 4, 3 };
static const char* com_name[] = { "com1", "com2", "com3", "com4" };

#define VGA_PATH "/bin/vga"
#define VGA_NAME "vga"

typedef struct {
  unsigned int interrupts[256];
  unsigned short com_ioport[4];
  unsigned short lpt_ioport[4];
  unsigned short equipment_list;
  unsigned char reserved0;
  unsigned short memory_size; /* In kilobytes. */
  unsigned char reserved1;
  unsigned char ps2_bios_control_flags;
  unsigned char keyboard_flag_byte0;
  unsigned char keyboard_flag_byte1;
  unsigned char alternate_keypad;
  unsigned short offset_to_buffer_head;
  unsigned short offset_to_buffer_tail;
  unsigned char keyboard_buffer[32];
  unsigned char drive_recalibration_status;
  unsigned char diskette_motor_status;
  unsigned char motor_shutoff_counter;
  unsigned char diskette_operation_status;
  unsigned char nec_diskette_controller_status[7];
  unsigned char current_video_mode;
  unsigned short screen_column_count;
  unsigned short video_regen_buffer_size;
  unsigned short current_video_page_offset;
  unsigned short cursor_position[8];
  unsigned char cursor_bottom_scanline;
  unsigned char cursor_top_scanline;
  unsigned char active_display_page;
  unsigned short crt_base_port;
  unsigned char crt_mode_control_register; /* Port 0x3c8 ? */
  unsigned char color_palette_mask; /* Port 0x3d9 */
  unsigned int ps2_reset_pointer;
  unsigned char reserved2;
  unsigned int daily_timer_counter;
  unsigned char clock_rollover_flag;
  unsigned char bios_break_flag;
  unsigned short soft_reset_flag;
  unsigned char hard_disk_last_status;
  unsigned char hard_disk_count;
  unsigned char xt_control_byte;
  unsigned char offet_to_fixed_disk_adapter;
  unsigned char lpt_timeout[4];
  unsigned char com_timeout[4];
  unsigned short keyboard_buffer_start_offset;
  unsigned short keyboard_buffer_end_offset;
  unsigned char rows_less1;
  unsigned short character_point_height;
  unsigned char video_mode_options;
  unsigned char ega_feature_bit_switches;
  unsigned char video_display_data_area;
  unsigned char display_combination_code;
  unsigned char last_diskette_data_rate;
  unsigned char hard_disk_status;
  unsigned char hard_disk_error;
  unsigned char hard_disk_interrupt_control;
  unsigned char hard_floppy_combo;
  unsigned char drive_state[4];
  unsigned char drive0_track;
  unsigned char drive1_track;
  unsigned char keyboard_mode_type;
  unsigned char keyboard_led_flags;
  unsigned int user_wait_complete_pointer;
  unsigned int user_wait_timeout;
  unsigned char rtc_wait_function_flag;
  unsigned char lana_dma_flags;
  unsigned char lana_status0;
  unsigned char lana_status1;
  unsigned int hard_disk_interrupt_vector;
  unsigned int video_parameter_table;
  unsigned int dynamic_save_area;
  unsigned int alpha_aux_char_generator;
  unsigned int text_aux_char_generator;
} __attribute__((packed)) bios_t;

#define INIT_NO 1
#define HALT_NO 2
#define VFS_RESPONSE_NO 3
#define VFS_REQUEST_NO 4

/* Initialization flag. */
static bool initialized = false;

static bd_t vfs_request_bda = -1;
static bd_t vfs_request_bdb = -1;
static vfs_request_queue_t vfs_request_queue;

static callback_queue_t vfs_response_queue;

#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "
#define INFO __FILE__ ": info: "

/* Context for creating serial ports. */
typedef struct {
  unsigned short port;
  unsigned char irq;
  const char* name;
} spc_t;

static spc_t*
spc_create (unsigned short port,
	    unsigned char irq,
	    const char* name)
{
  spc_t* spc = malloc (sizeof (spc_t));
  memset (spc, 0, sizeof (spc_t));
  spc->port = port;
  spc->irq = irq;
  spc->name = name;
  return spc;
}

static void
spc_destroy (spc_t* spc)
{
  free (spc);
}

static void
serial_port_callback (void* ptr,
		      bd_t bda,
		      bd_t bdb)
{
  spc_t* spc = ptr;

  vfs_error_t error;
  size_t size;
  if (read_vfs_readfile_response (bda, &error, &size) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read readfile response: %s", lily_error_string (lily_error));
    logs (log_buffer);
    spc_destroy (spc);
    return;
  }

  if (error != VFS_SUCCESS) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "vfs error: %s", vfs_error_string (error));
    logs (log_buffer);
    spc_destroy (spc);
    return;
  }

  bd_t bd = buffer_create (0);
  if (bd == -1) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create argument buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    spc_destroy (spc);
    return;
  }

  buffer_file_t bf;
  if (buffer_file_initw (&bf, bd) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize argument buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    spc_destroy (spc);
    return;
  }

  if (bfprintf (&bf, "port=%#x irq=%d", spc->port, spc->irq) < 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not write to argument buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    spc_destroy (spc);
    return;
  }

  if (creates (bdb, size, bd, -1, spc->name, true) == -1) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create serial port automaton: %s", lily_error_string (lily_error));
    logs (log_buffer);
    spc_destroy (spc);
    return;
  }

  if (buffer_destroy (bd) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy argument buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    spc_destroy (spc);
    return;
  }
  spc_destroy (spc);
}

static void
create_serial_port (unsigned short port,
		    unsigned char irq,
		    const char* name)
{
  spc_t* spc = spc_create (port, irq, name);
  if (vfs_request_queue_push_readfile (&vfs_request_queue, SERIAL_PORT_PATH) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not push vfs request: %s", lily_error_string (lily_error));
    logs (log_buffer);
    spc_destroy (spc);
  }
  if (callback_queue_push (&vfs_response_queue, serial_port_callback, spc) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not push vfs callback: %s", lily_error_string (lily_error));
    logs (log_buffer);
    spc_destroy (spc);
  }    
}

typedef struct {
  unsigned short port;
  unsigned int parameter_table;
} vgac_t;
static vgac_t vgac;

/* static void */
/* vga_callback (void* ptr, */
/* 	      bd_t bda, */
/* 	      bd_t bdb) */
/* { */
/*   vfs_error_t error; */
/*   size_t size; */
/*   if (read_vfs_readfile_response (0, bda, &error, &size) != 0) { */
/*     buffer_file_puts (&syslog_buffer, 0, ERROR "vfs provided bad readfile response\n"); */
/*     return; */
/*   } */

/*   if (error != VFS_SUCCESS) { */
/*     bfprintf (&syslog_buffer, ERROR "could not read %s\n", VGA_PATH); */
/*     return; */
/*   } */

/*   bd_t bd = buffer_create (0, 0); */
/*   if (bd == -1) { */
/*     buffer_file_puts (&syslog_buffer, 0, ERROR "could not create argument buffer\n"); */
/*     state = HALT; */
/*     return; */
/*   } */

/*   buffer_file_t bf; */
/*   if (buffer_file_initw (&bf, 0, bd) != 0) { */
/*     buffer_file_puts (&syslog_buffer, 0, ERROR "could not initialize argument buffer\n"); */
/*     state = HALT; */
/*     return; */
/*   } */

/*   if (bfprintf (&bf, "port=%#x parameter_table=%#x", vgac.port, vgac.parameter_table) < 0) { */
/*     buffer_file_puts (&syslog_buffer, 0, ERROR "could not write to argument buffer\n"); */
/*     state = HALT; */
/*     return; */
/*   } */

/*   if (creates (0, bdb, size, bd, -1, VGA_NAME, true) == -1) { */
/*     bfprintf (&syslog_buffer, ERROR "could not create %s\n", VGA_NAME); */
/*     state = HALT; */
/*     return; */
/*   } */

/*   buffer_destroy (0, bd); */
/* } */

static void
create_vga (unsigned short port,
	    unsigned int parameter_table)
{
  vgac.port = port;
  vgac.parameter_table = parameter_table;
  /* vfs_request_queue_push_readfile (&vfs_request_queue, 0, VGA_PATH); */
  /* callback_queue_push (&vfs_response_queue, 0, vga_callback, 0); */
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    aid_t aid = getaid ();

    vfs_request_bda = buffer_create (0);
    vfs_request_bdb = buffer_create (0);
    if (vfs_request_bda == -1 ||
    	vfs_request_bdb == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create vfs request buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    vfs_request_queue_init (&vfs_request_queue);
    callback_queue_init (&vfs_response_queue);

    /* Bind to the vfs. */
    aid_t vfs_aid = lookups (VFS_NAME);
    if (vfs_aid == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "vfs does not exist: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    
    description_t vfs_description;
    if (description_init (&vfs_description, vfs_aid) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create vfs description: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    action_desc_t vfs_request;
    if (description_read_name (&vfs_description, &vfs_request, VFS_REQUEST_NAME) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "vfs automaton does not contain request action");
      logs (log_buffer);
      exit (-1);
    }

    action_desc_t vfs_response;
    if (description_read_name (&vfs_description, &vfs_response, VFS_RESPONSE_NAME) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "vfs automaton does not contain response action");
      logs (log_buffer);
      exit (-1);
    }
    
    /* We bind the response first so they don't get lost. */
    if (bind (vfs_aid, vfs_response.number, 0, aid, VFS_RESPONSE_NO, 0) == -1 ||
    	bind (aid, VFS_REQUEST_NO, 0, vfs_aid, vfs_request.number, 0) == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not bind to vfs");
      logs (log_buffer);
      exit (-1);
    }

    if (description_fini (&vfs_description) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy vfs description: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    if (map (BDA_ADDRESS, BDA_ADDRESS, sizeof (bios_t)) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy vfs description: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    const bios_t* bios = BDA_ADDRESS;

    for (int idx = 0; idx != 4; ++idx) {
      if (bios->com_ioport[idx] != 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "BIOS reports a serial port at %#x", bios->com_ioport[idx]);
	logs (log_buffer);
	create_serial_port (bios->com_ioport[idx], com_irq[idx], com_name[idx]);
      }
    }

    if (bios->crt_base_port != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "BIOS reports a VGA at %#x", bios->crt_base_port);
      logs (log_buffer);
      create_vga (bios->crt_base_port, bios->video_parameter_table);
    }

    /* bfprintf (&syslog_buffer, INFO "lpt1=%x\n", bios->lpt_ioport[0]); */
    /* bfprintf (&syslog_buffer, INFO "lpt2=%x\n", bios->lpt_ioport[1]); */
    /* bfprintf (&syslog_buffer, INFO "lpt3=%x\n", bios->lpt_ioport[2]); */
    /* bfprintf (&syslog_buffer, INFO "lpt4=%x\n", bios->lpt_ioport[3]); */
    /* bfprintf (&syslog_buffer, INFO "current_video_mode=%x\n", bios->current_video_mode); */
    /* bfprintf (&syslog_buffer, INFO "screen_column_count=%d\n", bios->screen_column_count); */
    /* bfprintf (&syslog_buffer, INFO "crt_base_port=%x\n", bios->crt_base_port); */
    /* bfprintf (&syslog_buffer, INFO "crt_mode_control_register=%x\n", bios->crt_mode_control_register); */
    /* bfprintf (&syslog_buffer, INFO "rows_less1=%d\n", bios->rows_less1); */
    /* bfprintf (&syslog_buffer, INFO "character_point_height=%d\n", bios->character_point_height); */
    /* bfprintf (&syslog_buffer, INFO "video_mode_options=%x\n", bios->video_mode_options); */
    /* bfprintf (&syslog_buffer, INFO "ega_feature_bit_switches=%x\n", bios->ega_feature_bit_switches); */
    /* bfprintf (&syslog_buffer, INFO "video_display_data_area=%x\n", bios->video_display_data_area); */
    /* bfprintf (&syslog_buffer, INFO "display_combination_code=%x\n", bios->display_combination_code); */
    /* bfprintf (&syslog_buffer, INFO "video_parameter_table=%x\n", bios->video_parameter_table); */
    /* bfprintf (&syslog_buffer, INFO "dynamic_save_area=%x\n", bios->dynamic_save_area); */
    /* bfprintf (&syslog_buffer, INFO "alpha_aux_char_generator=%x\n", bios->alpha_aux_char_generator); */
    /* bfprintf (&syslog_buffer, INFO "text_aux_char_generator=%x\n", bios->text_aux_char_generator); */
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

/* vfs_request
   -------------
   Send a request to the vfs.

   Pre:  vfs_request_queue is not empty
   Post: vfs_request_queue has one item removed
 */
static bool
vfs_request_precondition (void)
{
  return !vfs_request_queue_empty (&vfs_request_queue);
}

BEGIN_OUTPUT (NO_PARAMETER, VFS_REQUEST_NO, "", "", vfs_request, ano_t ano, int param)
{
  initialize ();

  if (vfs_request_precondition ()) {
    if (vfs_request_queue_pop_to_buffer (&vfs_request_queue, vfs_request_bda, vfs_request_bdb) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not pop vfs request: %s", lily_error_string (lily_error));
      logs (log_buffer);
      finish_output (false, -1, -1);
    }
    finish_output (true, vfs_request_bda, vfs_request_bdb);
  }
  else {
    finish_output (false, -1, -1);
  }
}

/* vfs_response
   --------------
   Invoke the callback for a response from the vfs.

   Post: the callback queue has one item removed
 */
BEGIN_INPUT (NO_PARAMETER, VFS_RESPONSE_NO, "", "", vfs_response, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (callback_queue_empty (&vfs_response_queue)) {
    finish_input (bda, bdb);
  }

  const callback_queue_item_t* item = callback_queue_front (&vfs_response_queue);
  callback_t callback = callback_queue_item_callback (item);
  void* data = callback_queue_item_data (item);
  callback_queue_pop (&vfs_response_queue);

  callback (data, bda, bdb);

  finish_input (bda, bdb);
}

void
do_schedule (void)
{
  if (vfs_request_precondition ()) {
    if (schedule (VFS_REQUEST_NO, 0) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not schedule vfs request action: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
  }
}
