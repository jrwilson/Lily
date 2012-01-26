#include <action.h>
#include <finish.h>

void
init (void)
{

  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, 0, init);

void
produce (void)
{
  finish (0, 0, (const void*)1, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, 1, produce);
