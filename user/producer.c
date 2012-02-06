#include "producer.h"
#include <automaton.h>
#include <string.h>
#include <buffer_heap.h>
#include "terminal.h"

#define LIMIT 256
static int count = 0;

#define STRING_BUFFER_SIZE 256

void
producer_printer_op (const void* param,
		     size_t bc)
{
  if ((count < LIMIT) && bc != 0) {
    const size_t buffer_size = sizeof (terminal_display_arg_t) + STRING_BUFFER_SIZE;
    const bd_t bd = buffer_create (buffer_size);
    void* ptr = buffer_map (bd);
    buffer_heap_t heap;
    buffer_heap_init (&heap, ptr, buffer_size);

    terminal_display_arg_t* arg = buffer_heap_alloc (&heap, sizeof (terminal_display_arg_t));
    char* string = buffer_heap_alloc (&heap, STRING_BUFFER_SIZE);

    arg->size = snprintf (string, STRING_BUFFER_SIZE, "count = %d\r\n", count);
    arg->string = (void*)string - (void*)&arg->string;

    ++count;
    
    finish (PRODUCER_PRINTER_OP, 0, bd, FINISH_DESTROY);
  }
  else {
    finish (NO_ACTION, 0, -1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, PRODUCER_PRINTER_OP, producer_printer_op);
