#include "binding.hpp"
#include "automaton.hpp"

bool
binding::enabled () const
{
  return enabled_ && output_action.automaton->enabled () && input_action.automaton->enabled ();
}
