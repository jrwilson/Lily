#include "kput.h"
#include "kassert.h"

asm (".global bang_entry\n"
     "bang_entry:\n"
     "call driver");

static void driver ()
{
  kputs ("In "); kputs (__func__); kputs ("\n");
  kassert (0);
}
