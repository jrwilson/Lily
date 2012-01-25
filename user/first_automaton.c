#include <action.h>
#include <finish.h>

#define INIT_NAME "init"
#define INIT_DESCRIPTION ""
#define INIT_COMPARE NO_COMPARE
#define INIT_ACTION INTERNAL
#define INIT_PARAMETER NO_PARAMETER

void
init (void)
{

  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INIT, init);
