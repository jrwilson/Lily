#ifndef __io_h__
#define __io_h__

/*
  File
  ----
  io.h
  
  Description
  -----------
  Declarations for functions that manipulate I/O ports.

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/3.-The%20Screen.html
  Justin R. Wilson
*/

void
outb(unsigned short port,
     unsigned char value);

#endif /* __io_h__ */
