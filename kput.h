#ifndef __kput_h__
#define __kput_h__

/*
  File
  ----
  kput.h
  
  Description
  -----------
  Declarations for functions to print to the console.

  Authors:
  Justin R. Wilson
*/

#include "types.h"

void
kputs (const char* string);

void
kputx8 (uint8_t);

void
kputx16 (uint16_t);

void
kputx32 (uint32_t);

void
kputx64 (uint64_t);

void
kputp (const void* p);

void
clear_console (void);

#endif /* __kput_h__ */
