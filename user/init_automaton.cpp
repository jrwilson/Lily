#include <action_traits.hpp>

typedef np_b_nc_input_action_traits<void> init_traits;
extern "C" void
init (void)
{
  asm ("hlt\n");
}
ACTION (init_traits, init, init);
