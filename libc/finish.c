#include "automaton.h"

void
do_schedule (void);

void
finish_input (bd_t bda,
		  bd_t bdb)
{
  if (bda != -1) {
    buffer_destroy (bda);
  }
  if (bdb != -1) {
    buffer_destroy (bdb);
  }
  do_schedule ();
  finish (false, -1, -1);
}

void
finish_output (bool output_fired,
		   bd_t bda,
		   bd_t bdb)
{
  do_schedule ();
  finish (output_fired, bda, bdb);
}

void
finish_internal (void)
{
  do_schedule ();
  finish (false, -1, -1);
}
