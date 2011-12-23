#ifndef __privcall_hpp__
#define __privcall_hpp__

/*
  File
  ----
  privcall.hpp
  
  Description
  -----------
  The system automaton needs access to certain privileged instructions.

  Authors:
  Justin R. Wilson
*/

#include "vm_def.hpp"
#include "descriptor.hpp"

namespace privcall {
  
  enum {
    INVLPG,
  };

  inline void
  invlpg (logical_address_t address)
  {
    uint16_t cs;
    asm ("mov %%cs, %0\n" : "=g"(cs) ::);
    if ((cs & descriptor_constants::RING3) == descriptor_constants::RING0) {
      // Privileged mode.
      asm ("invlpg (%0)\n" :: "r"(address));
    }
    else {
      asm ("int $0x81\n" : : "a"(INVLPG), "b"(address) :);
    }
  }

}

#endif /* __privcall_hpp__ */
