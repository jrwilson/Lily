#include "producer.h"
#include <automaton.h>
#include <string.h>
#include "console.h"

const char* string = "Hello world!\rGoodbye world!\nHello world!\tHello world!\nOne\b\b\bTwo";

#define LIMIT 2
static int count = 0;

void
producer_printer_op (const void* param,
		     size_t bc)
{
  if ((count < LIMIT) && bc != 0) {
    ++count;

    size_t size = strlen (string);
    bd_t bd = buffer_create (size);
    void* data = buffer_map (bd);
    memcpy (data, string, size);
    finish (PRODUCER_PRINTER_OP, 0, bd, size, FINISH_DESTROY);
  }
  else {
    finish (NO_ACTION, 0, -1, 0, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, PRODUCER_PRINTER_OP, producer_printer_op);
