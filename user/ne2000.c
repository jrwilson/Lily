#include <automaton.h>
#include <buffer_file.h>
#include <string.h>
#include <description.h>
#include "pci.h"

/*
  Driver for ne2000 clones
  ========================

  
 */

#define INIT_NO 1

#define INFO "ne2000: info: "
#define ERROR "ne2000: error: "

/* Initialization flag. */
static bool initialized = false;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

void
do_schedule (void)
{
}
