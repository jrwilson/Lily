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

class console {
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

  console ();

  console&
  operator<< (short n)
  {
    print_signed (n);
    width_ = 0;
    return *this;
  }

  console&
  operator<< (int n)
  {
    print_signed (n);
    width_ = 0;
    return *this;
  }

  console&
  operator<< (long n)
  {
    print_signed (n);
    width_ = 0;
    return *this;
  }

  console&
  operator<< (unsigned short n)
  {
    print_integer (n, base ());
    width_ = 0;
    return *this;
  }

  console&
  operator<< (unsigned int n)
  {
    print_integer (n, base ());
    width_ = 0;
    return *this;
  }

  console&
  operator<< (unsigned long n)
  {
    print_integer (n, base ());
    width_ = 0;
    return *this;
  }

  console&
  operator<< (float n);

  console&
  operator<< (double n);

  console&
  operator<< (long double n);

  console&
  operator<< (bool n);

  console&
  operator<< (const void* p)
  {
    *this << reinterpret_cast<uintintptr_t> (p);
    width_ = 0;
    return *this;
  }

  console&
  operator<< (console& (*ptr) (console&))
  {
    ptr (*this);
    return *this;
  }

  console&
  put (char c);

  console&
  write (const char* s,
	 size_t n);

  fmtflags
  flags () const
  {
    return flags_;
  }

  fmtflags
  flags (fmtflags fmt)
  {
    fmtflags r = flags_; flags_ = fmt; return r;
  }

  fmtflags
  setf (fmtflags fmt)
  {
    return flags (flags () | fmt);
  }

  fmtflags
  setf (fmtflags fmt, fmtflags mask)
  {
    return flags ((flags () & ~mask) | (fmt & mask));
  }

  void
  unsetf (fmtflags mask)
  {
    flags (flags () & ~mask);
  }

  void
  flush ()
  {
    /* Do nothing.  Unbuffered. */
  }

  streamsize
  width () const
  {
    return width_;
  }

  streamsize
  width (streamsize w)
  {
    std::swap (w, width_);
    return w;
  }

  char
  fill () const
  {
    return fill_;
  }

  char
  fill (char c)
  {
    std::swap (c, fill_);
    return c;
  }

private:
  // Width and height of the framebuffer.
  static const unsigned int WIDTH  = 80;
  static const unsigned int HEIGHT = 25;

  unsigned short* const videoram_;

  char left_buffer_[3];
  size_t left_buffer_size_;

  char right_buffer_[20];
  size_t right_buffer_size_;

  unsigned int x_location_;
  unsigned int y_location_;
  fmtflags flags_;
  streamsize width_;
  char fill_;

  void
  push_left (char c)
  {
    left_buffer_[left_buffer_size_++] = c;
  }

  void
  print_left ()
  {
    for (size_t idx = 0; idx < left_buffer_size_; ++idx) {
      put (left_buffer_[idx]);
    }
  }

  void
  push_right (char c)
  {
    right_buffer_[right_buffer_size_++] = c;
  }

  void
  print_right ()
  {
    size_t x = right_buffer_size_;
    while (x != 0) {
      put (right_buffer_[--x]);
    }
  }

  void
  pad ()
  {
    for (int idx = left_buffer_size_ + right_buffer_size_; idx < width_; ++idx) {
      put (fill_);
    }
  }

  int_fast8_t
  base () const
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
  
  char
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
  void
  print_signed (T n)
  {
    if (n < 0) {
      push_left ('-');
      n = -n;
    }
    print_integer (n, base ());
  }

  template <typename T>
  void
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
  void
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
