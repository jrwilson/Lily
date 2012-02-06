#include "producer.h"
#include <automaton.h>
#include <string.h>
#include <buffer_heap.h>
#include "terminal.h"

const char* message = "Hello world!\n\r";

#define LIMIT 15
static int count = 0;

void
producer_printer_op (const void* param,
		     size_t bc)
{
  if ((count < LIMIT) && bc != 0) {
    ++count;

    const size_t message_size = strlen (message);
    const size_t buffer_size = sizeof (terminal_display_arg_t) + message_size;
    const bd_t bd = buffer_create (buffer_size);
    void* ptr = buffer_map (bd);
    buffer_heap_t heap;
    buffer_heap_init (&heap, ptr, buffer_size);

    terminal_display_arg_t* arg = buffer_heap_alloc (&heap, sizeof (terminal_display_arg_t));
    char* string = buffer_heap_alloc (&heap, message_size);
    memcpy (string, message, message_size);

    arg->size = message_size;
    arg->string = (void*)string - (void*)&arg->string;
    
    finish (PRODUCER_PRINTER_OP, 0, bd, FINISH_DESTROY);
  }
  else {
    finish (NO_ACTION, 0, -1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, PRODUCER_PRINTER_OP, producer_printer_op);
