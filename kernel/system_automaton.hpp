#ifndef __system_automaton_hpp__
#define __system_automaton_hpp__

#include "shared_ptr.hpp"
#include "lily/types.h"

class automaton;

extern shared_ptr<automaton> system_automaton;
extern bd_t boot_data;

#endif /* __system_automaton_hpp__ */
