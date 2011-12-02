#ifndef __vm_def_hpp__
#define __vm_def_hpp__

#include "types.hpp"

#define PAGE_SIZE 0x1000

/* Should agree with loader.s. */
const logical_address KERNEL_VIRTUAL_BASE (reinterpret_cast<const void*> (0xC0000000));

/* Everything must fit into 4MB initialy. */
const logical_address INITIAL_LOGICAL_LIMIT (reinterpret_cast<const void*> (0xC0400000));

#endif /* __vm_def_hpp__ */
