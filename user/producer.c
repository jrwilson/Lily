#include "producer.h"
#include <automaton.h>
#include <string.h>

#define LIMIT 3
static int count = 0;

const char* string = "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n";

void
init (void)
{
  /* Schedule produce. */
  finish (PRODUCER_PRINT, 0, -1, 0, 0);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, LILY_ACTION_INIT, init);

void
print (void)
{
  if (count < LIMIT) {
    ++count;
    size_t len = strlen (string);
    bd_t bd = buffer_create (len);
    char* buffer = buffer_map (bd);
    memcpy (buffer, string, len);
    finish (PRODUCER_PRINT, 0, bd, len, FINISH_DESTROY);
  }
  else {
    finish (NO_ACTION, 0, -1, 0, 0);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, PRODUCER_PRINT, print);

void
print_sense (void)
{
  finish (NO_ACTION, 0, -1, 0, 0);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, PRODUCER_PRINT_SENSE, print_sense);
