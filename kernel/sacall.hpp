#ifndef __sacall_hpp__
#define __sacall_hpp__

/*
  File
  ----
  sacall.hpp
  
  Description
  -----------
  The system automaton needs access to certain privileged instructions.

  Authors:
  Justin R. Wilson
*/

#include "vm_def.hpp"
#include "descriptor.hpp"

namespace sacall {
  
  enum {
    INVLPG,
  };

  inline void
  invlpg (logical_address_t address)
  {
    uint16_t cs;
    asm ("mov %%cs, %0\n" : "=g"(cs) ::);
    if ((cs & descriptor::RING3) == descriptor::RING0) {
      // Privileged mode.
      asm ("invlpg (%0)\n" :: "r"(address));
    }
    else {
      asm ("int $0x82\n" : : "a"(INVLPG), "b"(address) :);
    }
  }

}

#endif /* __sacall_hpp__ */
