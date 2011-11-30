#ifndef __vm_def_hpp__
#define __vm_def_hpp__

#include "types.hpp"

#define PAGE_SIZE 0x1000

/* Should agree with loader.s. */
static const logical_address KERNEL_VIRTUAL_BASE (reinterpret_cast<void*> (0xC0000000));

#endif /* __vm_def_hpp__ */
