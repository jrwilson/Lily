#include <automaton.h>
#include <io.h>
#include <string.h>
#include <buffer_queue.h>
#include <fifo_scheduler.h>
#include "argv.h"

#define DESTROY_BUFFERS_NO 1

/* Initialization flag. */
static bool initialized = false;

/* Queue of buffers that need to be destroyed. */
static buffer_queue_t destroy_queue;

static void
ssyslog (const char* msg)
{
  syslog (msg, strlen (msg));
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    buffer_queue_init (&destroy_queue);
  }
}

static void
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb);

/* init
   ----
   The script to execute comes in on bda.

   Post: ???
 */
BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t aid, bd_t bda, bd_t bdb)
{
  ssyslog ("jsh: init\n");
  initialize ();

  argv_t argv;
  size_t count;
  if (argv_initr (&argv, bda, bdb, &count) != -1) {
    for (size_t i = 0; i != count; ++i) {
      const char* arg = argv_arg (&argv, i);
      ssyslog (arg);
      ssyslog ("\n");
    }
  }

  end_action (false, bda, bdb);
}

/* destroy_buffers
   ---------------
   Destroys all of the buffers in destroy_queue.
   This is useful for output actions that need to destroy the buffer *after* the output has fired.
   To schedule a buffer for destruction, just add it to destroy_queue.

   Pre:  Destroy queue is not empty.
   Post: Destroy queue is empty.
 */
static bool
destroy_buffers_precondition (void)
{
  return !buffer_queue_empty (&destroy_queue);
}

BEGIN_INTERNAL (NO_PARAMETER, DESTROY_BUFFERS_NO, "", "", destroy_buffers, int param)
{
  initialize ();
  scheduler_remove (DESTROY_BUFFERS_NO, param);

  if (destroy_buffers_precondition ()) {
    while (!buffer_queue_empty (&destroy_queue)) {
      bd_t bd;
      const buffer_queue_item_t* item = buffer_queue_front (&destroy_queue);
      bd = buffer_queue_item_bda (item);
      if (bd != -1) {
	buffer_destroy (bd);
      }
      bd = buffer_queue_item_bdb (item);
      if (bd != -1) {
	buffer_destroy (bd);
      }

      buffer_queue_pop (&destroy_queue);
    }
  }

  end_action (false, -1, -1);
}

/* end_action is a helper function for terminating actions.
   If the buffer is not -1, it schedules it to be destroyed.
   end_action schedules local actions and calls scheduler_finish to finish the action.
*/
static void
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb)
{
  if (bda != -1 || bdb != -1) {
    buffer_queue_push (&destroy_queue, 0, bda, bdb);
  }

  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }

  scheduler_finish (output_fired, bda, bdb);
}

/* #include "keyboard.h" */
/* #include "kb_us_104.h" */
/* #include "shell.h" */
/* #include "terminal.h" */
/* #include "vga.h" */

  /* aid_t keyboard = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "keyboard") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     keyboard = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t kb_us_104 = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "kb_us_104") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     kb_us_104 = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t shell = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "shell") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     shell = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t terminal = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "terminal") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     terminal = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t vga = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "vga") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     vga = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* bind (keyboard, KEYBOARD_SCAN_CODE, 0, kb_us_104, KB_US_104_SCAN_CODE, 0); */
  /* bind (kb_us_104, KB_US_104_STRING, 0, shell, SHELL_STDIN, 0); */
  /* bind (shell, SHELL_STDOUT, 0, terminal, TERMINAL_DISPLAY, 0); */
  /* bind (terminal, TERMINAL_VGA_OP, 0, vga, VGA_OP, 0); */
