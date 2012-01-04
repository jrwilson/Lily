#include <action_traits.hpp>

typedef action_traits<input_action, no_parameter, null_type, void> init_traits;
#define INIT_TRAITS init_traits
#define INIT_NAME "init"
#define INIT_DESCRIPTION "description"
#define INIT_ACTION M_INPUT
#define INIT_PARAMETER M_NO_PARAMETER

extern "C" void
init (void)
{
  asm ("hlt\n");
}
ACTION_DESCRIPTOR (INIT, init);

