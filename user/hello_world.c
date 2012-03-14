#include <automaton.h>
#include <fifo_scheduler.h>
#include <buffer_file.h>

/*
  Begin Automaton Section
  =======================
*/

#define START_NO 1
#define STDOUT_NO 2
#define DONE_NO 3

typedef enum {
  START,
  EXECUTING,
} state_t;
 
static state_t state = START;

/* Initialization flag. */
static bool initialized = false;

/* Text to output. */
static bd_t stdout_bd;
static buffer_file_t stdout_bf;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    stdout_bd = buffer_create (0);
    if (stdout_bd == -1) {
      syslog ("jsh: error: Could not create stdout buffer");
      exit ();
    }
    if (buffer_file_initw (&stdout_bf, stdout_bd) != 0) {
      syslog ("jsh: error: Could not initialize stdout buffer");
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

static void
end_internal_action (void)
{
  schedule ();
  scheduler_finish (false, -1, -1);
}

/* start
   -----
   Start the automaton.
   
   Post: if state == START then state == EXECUTING
 */
BEGIN_INPUT (NO_PARAMETER, START_NO, "start", "", start, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (state == START) {
    state = EXECUTING;
    bfprintf (&stdout_bf, "Hello World!\n");
  }

  end_input_action (bda, bdb);
}

/* stdout
   ------
   Outgoing text.
   
   Pre:  the output buffer is not empty
   Post: the output buffer appears empty
 */
static bool
stdout_precondition (void)
{
  return buffer_file_size (&stdout_bf) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, STDOUT_NO, "stdout", "buffer_file", stdout, int param)
{
  initialize ();
  scheduler_remove (STDOUT_NO, param);

  if (stdout_precondition ()) {
    buffer_file_truncate (&stdout_bf);
    end_output_action (true, stdout_bd, -1);
  }
  else {
    end_output_action (false, -1, -1);
  }
}

/* done
   ----
   Destroy the automaton.
   
   Pre:  state == DONE && the output buffer is empty
   Post: exit
 */
static bool
done_precondition (void)
{
  return state == EXECUTING && buffer_file_size (&stdout_bf) == 0;
}

BEGIN_INTERNAL (NO_PARAMETER, DONE_NO, "", "", done, int param)
{
  initialize ();
  scheduler_remove (DONE_NO, param);

  if (done_precondition ()) {
    exit ();
  }
  else {
    end_internal_action ();
  }
}

static void
schedule (void)
{
  if (stdout_precondition ()) {
    scheduler_add (STDOUT_NO, 0);
  }
  if (done_precondition ()) {
    scheduler_add (DONE_NO, 0);
  }
}

/*
  End Automaton Section
  =======================
*/
