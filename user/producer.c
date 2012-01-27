#include <action.h>
#include <finish.h>

#define LIMIT 3
static int count = 0;

#define PRODUCE 1

void
init (void)
{
  /* Schedule produce. */
  finish (PRODUCE, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, LILY_ACTION_INIT, init);

void
produce (void)
{
  if (count < LIMIT) {
    ++count;
    finish (PRODUCE, 0, "Hello\n", 6, -1, 0);
  }
  else {
    finish (0, 0, 0, 0, -1, 0);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, PRODUCE, produce);
