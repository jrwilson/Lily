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
#include <stdint.h>
#include <algorithm>
#include "registers.hpp"

// TODO:  Perhpas this could be simpler to match its intended use.

class console {
private:
  static inline unsigned short*
  videoram ()
  {
    return reinterpret_cast<unsigned short*> (0xB8000);
  }

  // Colors.
  static const unsigned char BLACK = 0;
  static const unsigned char BLUE = 1;
  static const unsigned char GREEN = 2;
  static const unsigned char CYAN = 3;
  static const unsigned char RED = 4;
  static const unsigned char MAGENTA = 5;
  static const unsigned char BROWN = 6;
  static const unsigned char LIGHT_GREY = 7;
  static const unsigned char DARK_GREY = 8;
  static const unsigned char LIGHT_BLUE = 9;
  static const unsigned char LIGHT_GREEN = 10;
  static const unsigned char LIGHT_CYAN = 11;
  static const unsigned char LIGHT_RED = 12;
  static const unsigned char LIGHT_MAGENTA = 13;
  static const unsigned char LIGHT_BROWN = 14;
  static const unsigned char WHITE = 15;
  
public:
  typedef int streamsize;
  typedef uint_fast16_t fmtflags;

  static const fmtflags left = (1 << 0);
  static const fmtflags right = (1 << 1);
  static const fmtflags internal = (1 << 2);
  static const fmtflags adjustfield = (7 << 0);

  static const fmtflags dec = (1 << 3);
  static const fmtflags hex = (1 << 4);
  static const fmtflags oct = (1 << 5);
  static const fmtflags basefield = (7 << 3);

  static const fmtflags scientific = (1 << 6);
  static const fmtflags fixed = (1 << 7);
  static const fmtflags floatfield = (3 << 6);

  static const fmtflags skipws = (1 << 8);
  static const fmtflags boolalpha = (1 << 9);
  static const fmtflags showbase = (1 << 10);
  static const fmtflags showpoint = (1 << 11);
  static const fmtflags showpos = (1 << 12);
  static const fmtflags uppercase = (1 << 13);
  static const fmtflags unitbuf = (1 << 14);

  static void
  initialize ()
  {
    flags_ = (right | dec | skipws);
    fill_ = ' ';
    for (unsigned int y = 0; y < HEIGHT; ++y) {
      for (unsigned int x = 0; x < WIDTH; ++x) {
  	videoram ()[y * WIDTH + x] = (BLACK << 12) | (WHITE << 8) | ' ';
      }
    }
  }

  inline console&
  operator<< (short n)
  {
    print_signed (n);
    width_ = 0;
    return *this;
  }

  inline console&
  operator<< (int n)
  {
    print_signed (n);
    width_ = 0;
    return *this;
  }

  inline console&
  operator<< (long n)
  {
    print_signed (n);
    width_ = 0;
    return *this;
  }

  inline console&
  operator<< (unsigned short n)
  {
    print_integer (n, base ());
    width_ = 0;
    return *this;
  }

  inline console&
  operator<< (unsigned int n)
  {
    print_integer (n, base ());
    width_ = 0;
    return *this;
  }

  inline console&
  operator<< (unsigned long n)
  {
    print_integer (n, base ());
    width_ = 0;
    return *this;
  }

  inline console&
  operator<< (float n);

  inline console&
  operator<< (double n);

  inline console&
  operator<< (long double n);

  inline console&
  operator<< (bool n)
  {
    if (n) {
      put ('1');
    }
    else {
      put ('0');
    }
    return *this;
  }

  inline console&
  operator<< (const void* p)
  {
    *this << reinterpret_cast<uintptr_t> (p);
    width_ = 0;
    return *this;
  }

  inline console&
  operator<< (console& (*ptr) (console&))
  {
    ptr (*this);
    return *this;
  }

  inline console&
  put (char c)
  {
    // TODO:  Can we scroll with hardware?
    /* Scroll if we are at the bottom of the screen. */
    if (y_location_ == HEIGHT) {
      unsigned int y;
      for (y = 0; y < HEIGHT - 1; ++y) {
	for (unsigned int x = 0; x < WIDTH; ++x) {
	  videoram ()[y * WIDTH + x] = videoram ()[(y + 1) * WIDTH + x];
	}
      }
      /* Fill last line with spaces. */
      for (unsigned int x = 0; x < WIDTH; ++x) {
	videoram ()[y * WIDTH + x] = (BLACK << 12) | (WHITE << 8) | ' ';
      }
      --y_location_;
    }
    
    switch (c) {
    case '\b':
      if (x_location_ > 0) {
	--x_location_;
      }
      break;
    case '\t':
      /* A tab is a position divisible by 8. */
      x_location_ = (x_location_ + 8) & ~(8-1);
      break;
    case '\n':
      x_location_ = 0;
      ++y_location_;
      break;
    case '\r':
      x_location_ = 0;
      break;
    default:
      /* Print the character using black on white. */
      videoram ()[y_location_ * WIDTH + x_location_] = (BLACK << 12) | (WHITE << 8) | c;
      /* Advance the cursor. */
      ++x_location_;
      if (x_location_ == WIDTH) {
	++y_location_;
	x_location_ = 0;
      }
    }
    
    return *this;
  }

  static inline console&
  write (const char* s,
	 size_t n);

  static inline fmtflags
  flags ()
  {
    return flags_;
  }

  static inline fmtflags
  flags (fmtflags fmt)
  {
    fmtflags r = flags_; flags_ = fmt; return r;
  }

  static inline fmtflags
  setf (fmtflags fmt)
  {
    return flags (flags () | fmt);
  }

  static inline fmtflags
  setf (fmtflags fmt, fmtflags mask)
  {
    return flags ((flags () & ~mask) | (fmt & mask));
  }

  static inline void
  unsetf (fmtflags mask)
  {
    flags (flags () & ~mask);
  }

  static inline void
  flush ()
  {
    /* Do nothing.  Unbuffered. */
  }

  static inline streamsize
  width ()
  {
    return width_;
  }

  static inline streamsize
  width (streamsize w)
  {
    std::swap (w, width_);
    return w;
  }

  static inline char
  fill ()
  {
    return fill_;
  }

  static inline char
  fill (char c)
  {
    std::swap (c, fill_);
    return c;
  }

private:
  // Width and height of the framebuffer.
  static const unsigned int WIDTH  = 80;
  static const unsigned int HEIGHT = 25;

  static char left_buffer_[3];
  static size_t left_buffer_size_;

  static char right_buffer_[20];
  static size_t right_buffer_size_;

  static unsigned int x_location_;
  static unsigned int y_location_;
  static fmtflags flags_;
  static streamsize width_;
  static char fill_;

  static inline void
  push_left (char c)
  {
    left_buffer_[left_buffer_size_++] = c;
  }

  inline void
  print_left ()
  {
    for (size_t idx = 0; idx < left_buffer_size_; ++idx) {
      put (left_buffer_[idx]);
    }
  }

  static inline void
  push_right (char c)
  {
    right_buffer_[right_buffer_size_++] = c;
  }

  inline void
  print_right ()
  {
    size_t x = right_buffer_size_;
    while (x != 0) {
      put (right_buffer_[--x]);
    }
  }

  inline void
  pad ()
  {
    for (int idx = left_buffer_size_ + right_buffer_size_; idx < width_; ++idx) {
      put (fill_);
    }
  }

  static inline int_fast8_t
  base ()
  {
    switch (flags_ & basefield) {
    case dec:
      return 10;
    case hex:
      return 16;
    case oct:
      return 8;
    default:
      return 10;
    }
  }
  
  static inline char
  digit (int_fast8_t value)
  {
    switch (flags_ & basefield) {
    case dec:
    case oct:
      return value + '0';
    case hex:
      if ((flags_ & basefield) == hex) {
	if (value < 10) {
	  return value + '0';
	}
	else {
	  return value - 10 + ((flags_ & uppercase) ? 'A' : 'a');
	}
      }
    }
    return value + '0';
  }

  template <typename T>
  inline void
  print_signed (T n)
  {
    if (n < 0) {
      push_left ('-');
      n = -n;
    }
    print_integer (n, base ());
  }

  template <typename T>
  inline void
  print_integer (T n,
		 int_fast8_t base)
  {
    if (flags_ & showbase) {
      switch (flags_ & basefield) {
      case dec:
	break;
      case oct:
	push_left ('0');
	break;
      case hex:
	push_left ('0');
	push_left ((flags_ & uppercase) ? 'X' : 'x');
	break;
      }
    }

    push_digits (n, base);

    if (width_ == 0) {
      print_left ();
      print_right ();
    }
    else {
      switch (flags_ & adjustfield) {
      case left:
	pad ();
	print_left ();
	print_right ();
	break;
      case internal:
	print_left ();
	pad ();
	print_right ();
	break;
      case right:
	print_left ();
	print_right ();
	pad ();
	break;
      }
    }

    left_buffer_size_ = 0;
    right_buffer_size_ = 0;
  }

  template <typename T>
  static inline void
  push_digits (T n,
	       int_fast8_t base)
  {
    do {
      const T rem = n % base;
      push_right (digit (rem));
      n /= base;
    } while (n != 0);
  }

};

inline console&
operator<< (console& con,
	    char c)
{
  return con.put (c);
}

console&
operator<< (console&,
	    signed char);

console&
operator<< (console&,
	    unsigned char);

inline console&
operator<< (console& c,
	    const char* s)
{
  for (; *s != 0; ++s) {
    c.put (*s);
  }
  
  return c;
}

console&
operator<< (console&,
	    const signed char*);

console&
operator<< (console&,
	    const unsigned char*);

inline console&
endl (console& c)
{
  c.put ('\n');
  c.flush ();
  return c;
}

inline console&
hex (console& c)
{
  c.setf (console::hex, console::basefield);
  return c;
}

inline console&
showbase (console& c)
{
  c.setf (console::showbase);
  return c;
}

inline console&
left (console& c)
{
  c.setf (console::left, console::adjustfield);
  return c;
}

inline console&
internal (console& c)
{
  c.setf (console::internal, console::adjustfield);
  return c;
}

inline console&
right (console& c)
{
  c.setf (console::right, console::adjustfield);
  return c;
}

// From the C++ Programming Language (Stroustrup p. 633).
struct smanip {
  console& (*f) (console&, int);
  int i;
  smanip (console& (*ff) (console&, int), int ii) : f (ff), i (ii) { }
};

inline console&
operator<< (console& c,
	    const smanip& s)
{
  return s.f (c, s.i);
}

inline console&
set_width (console& c,
	   int n)
{
  c.width (n);
  return c;
}

inline smanip
setw (int n) 
{
  return smanip (set_width, n);
}

inline console&
set_fill (console& c,
	  int n)
{
  c.fill (n);
  return c;
}

inline smanip
setfill (char c) 
{
  return smanip (set_fill, c);
}

template <typename T>
struct hex_format {
  T value;

  hex_format (T v) :
    value (v)
  { }
};

template <typename T>
inline hex_format<T>
hexformat (T v)
{
  return hex_format<T> (v);
}

template <typename T>
console&
operator<< (console& c,
	    const hex_format<T>& hf)
{
  console::fmtflags flags = c.flags ();
  c << hex << showbase << internal << setw (10) << setfill ('0') << hf.value;
  c.flags (flags);
  return c;
}

inline console&
operator<< (console& c,
	    volatile registers& regs)
{
  c << "Number: " << regs.number << " Error: " << hexformat (regs.error) << " EFLAGS: " << hexformat (regs.eflags) << endl;
  c << "EAX: " << hexformat (regs.eax) << " EBX: " << hexformat (regs.ebx) << " ECX: " << hexformat (regs.ecx) << " EDX: " << hexformat (regs.edx) << endl;
  c << "ESP: " << hexformat (regs.esp) << " EBP: " << hexformat (regs.ebp) << " ESI: " << hexformat (regs.esi) << " EDI: " << hexformat (regs.edi) << endl;
  c << " DS: " << hexformat (regs.ds) << "  CS: " << hexformat (regs.cs) << " EIP: " << hexformat (regs.eip) << endl;
  c << "USERSP: " << hexformat (regs.useresp) << " USERSS: " << hexformat (regs.ss) << endl;

  return c;
}

extern console kout;

#endif /* __kout_hpp__ */
