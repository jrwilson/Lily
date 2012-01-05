#include <action_traits.hpp>

typedef action_traits<input_action, no_parameter> init_traits;
#define INIT_TRAITS init_traits
#define INIT_NAME "init"
#define INIT_DESCRIPTION "description"
#define INIT_ACTION M_INPUT
#define INIT_PARAMETER M_NO_PARAMETER

extern "C" void
init (no_param_t, void*, size_t, bid_t, size_t)
{
  asm ("hlt\n");
}
ACTION_DESCRIPTOR (INIT, init);
