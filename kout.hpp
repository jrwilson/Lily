#ifndef __kout_hpp__
#define __kout_hpp__

/*
  File
  ----
  kout.h
  
  Description
  -----------
  Console output.

  Authors:
  Justin R. Wilson
*/

#include <stddef.h>

class console {
private:
  // Width and height of the framebuffer.
  static const unsigned int WIDTH  = 80;
  static const unsigned int HEIGHT = 25;

  unsigned short* const videoram_;

  unsigned int x_location_;
  unsigned int y_location_;

public:
  console ();

  console& operator<< (short n);
  console& operator<< (int n);
  console& operator<< (long n);

  console& operator<< (unsigned short n);
  console& operator<< (unsigned int n);
  console& operator<< (unsigned long n);

  console& operator<< (float n);
  console& operator<< (double n);
  console& operator<< (long double n);

  console& operator<< (bool n);
  console& operator<< (const void* p);

  console& operator<< (console& (*ptr) (console&));

  console& put (char c);
  console& write (const char* s,
			 size_t n);

  void flush ();
};

console&
operator<< (console&,
	    char);

console&
operator<< (console&,
	    signed char);

console&
operator<< (console&,
	    unsigned char);

console&
operator<< (console&,
	    const char*);

console&
operator<< (console&,
	    const signed char*);

console&
operator<< (console&,
	    const unsigned char*);

console&
endl (console&);

extern console kout;

#endif /* __kout_hpp__ */
