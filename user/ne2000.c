#include <automaton.h>
#include <buffer_file.h>
#include <string.h>
#include <description.h>
#include "syslog.h"
#include "pci.h"

/*
  Driver for ne2000 clones
  ========================

  
 */

#define INIT_NO 1
#define STOP_NO 2
#define SYSLOG_NO 3

#define INFO "ne2000: info: "
#define ERROR "ne2000: error: "

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

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    syslog_bd = buffer_create (0, 0);
    if (syslog_bd == -1) {
      /* Nothing we can do. */
      exit ();
    }
    if (buffer_file_initw (&syslog_buffer, 0, syslog_bd) != 0) {
      /* Nothing we can do. */
      exit ();
    }

    aid_t syslog_aid = lookup (0, SYSLOG_NAME, strlen (SYSLOG_NAME) + 1);
    if (syslog_aid != -1) {
      /* Bind to the syslog. */

      description_t syslog_description;
      if (description_init (&syslog_description, 0, syslog_aid) != 0) {
	exit ();
      }
      
      action_desc_t syslog_text_in;
      if (description_read_name (&syslog_description, &syslog_text_in, SYSLOG_TEXT_IN) != 0) {
	exit ();
      }
      
      /* We bind the response first so they don't get lost. */
      if (bind (0, getaid (), SYSLOG_NO, 0, syslog_aid, syslog_text_in.number, 0) == -1) {
	exit ();
      }

      description_fini (&syslog_description, 0);
    }

    /* Lookup the PCI automaton. */
    aid_t pci_aid = lookup (0, PCI_NAME, strlen (PCI_NAME) + 1);
    if (pci_aid == -1) {
      bfprintf (&syslog_buffer, 0, ERROR "pci automaton does not exist\n");
      state = STOP;
      return;
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
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

  if (stop_precondition ()) {
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

void
do_schedule (void)
{
  if (stop_precondition ()) {
    schedule (0, STOP_NO, 0);
  }
  if (syslog_precondition ()) {
    schedule (0, SYSLOG_NO, 0);
  }
}
