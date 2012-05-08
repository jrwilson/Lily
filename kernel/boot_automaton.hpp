#ifndef __system_automaton_hpp__
#define __system_automaton_hpp__

#include "lily/types.h"
#include "shared_ptr.hpp"

class automaton;

extern shared_ptr<automaton> boot_automaton;
extern bd_t boot_data;

#endif /* __system_automaton_hpp__ */
