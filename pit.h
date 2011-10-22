#ifndef __pit_h__
#define __pit_h__

/*
  File
  ----
  pit.h
  
  Description
  -----------
  Declarations for functions to manage the programmable interrupt timer (pit).

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/5.-IRQs%20and%20the%20PIT.html
  http://en.wikipedia.org/wiki/Intel_8253
  Justin R. Wilson
*/

void
initialize_pit (unsigned short period);

#endif /* __pit_h__ */
