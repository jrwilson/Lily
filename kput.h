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

void
kputs (const char* string);

void
kputucx (unsigned char n);

void
kputuix (unsigned int n);

void
kputp (void* p);

void
clear_console ();

#endif /* __kput_h__ */
