#include "shell.h"
#include <automaton.h>
#include <string.h>
#include <buffer_heap.h>
#include "terminal.h"
#include "keyboard.h"
#include <fifo_scheduler.h>
#include <dymem.h>

/*
  Shell
  =====
  A text-based shell for the Lily kernel.
  The shell allows a user to create and manipulate a constellation of automata and in this respect resembles an interactive editor.

  Design
  ------
  The design will be structured around the Model-View-Controller pattern.
  The model consists of the automata and bindings that form the constellation.
  The view consists of various windows, diagrams, etc. that describe automata, bindings, and constellation.
  The controller translates input (from a keyboard) to commands to manipulate the model and/or view.

  Local IDs vs. Global IDs
  ------------------------
  Automata (and possibly bindings) have global IDs that can be drawn from a large set.
  I believe it would be useful to remap the global IDs into a smaller set of local IDs.
  The user would manipulate automata and bindings using the local IDs.
  This should save them some typing and reduces mistakes because its certainly easier to remember a one or two digit number than it is a six or ten digit number.

 */

typedef struct automaton_struct automaton_t;
typedef struct action_struct action_t;
typedef struct binding_struct binding_t;

struct action_struct {
  automaton_t* automaton;	/* The associated automaton. */
  ano_t ano;			/* The action number. */
  int lid;			/* The local action number. */
  action_t* next;
};

struct automaton_struct {
  aid_t aid;		/* The global identifier for this automaton. */
  int lid;		/* The local identifier for this automaton. */
  action_t* actions;	/* The list of actions. */
  automaton_t* next;
};

struct binding_struct {
  bid_t bid;			/* The global identifier for this binding. */
  action_t* output_action;	/* The output action and parameter. */
  int output_parameter;
  action_t* input_action;	/* The input action and parameter. */
  int input_parameter;
  binding_t* next;
};








typedef struct item item_t;
struct item {
  bd_t bd;
  item_t* next;
};

static item_t* head = 0;
static item_t** tail = &head;
static size_t shell_display_binding_count = 1;

static bool
empty (void)
{
  return head == 0;
}

static void
push (item_t* item)
{
  item->next = 0;
  *tail = item;
  tail = &item->next;
}

static item_t*
pop (void)
{
  item_t* retval = head;
  head = retval->next;
  if (head == 0) {
    tail = &head;
  }
  return retval;
}

static void
schedule (void);

void
init (int param,
      bd_t bd,
      void* ptr,
      size_t buffer_size)
{
  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);

void
shell_string (int param,
		 bd_t bd,
		 void* ptr,
		 size_t size)
{
  if (bd != -1) {
    item_t* item = malloc (sizeof (item_t));
    item->bd = bd;
    push (item);
  }

  schedule ();
  scheduler_finish (bd, FINISH_NO);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, SHELL_STRING, shell_string);

static bool
shell_display_precondition (void)
{
  return !empty () && shell_display_binding_count != 0;
}

void
shell_display (int param,
		  size_t bc)
{
  scheduler_remove (SHELL_DISPLAY, param);
  shell_display_binding_count = bc;

  if (shell_display_precondition ()) {
    item_t* item = pop ();
    bd_t bd = item->bd;
    free (item);

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, SHELL_DISPLAY, shell_display);

static void
schedule (void)
{
  if (shell_display_precondition ()) {
    scheduler_add (SHELL_DISPLAY, 0);
  }
}
