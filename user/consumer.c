#include <action.h>
#include <finish.h>

void
init (void)
{
  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, 0, init);

void
consume (void)
{
  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, 1, consume);
