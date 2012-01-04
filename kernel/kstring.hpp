#ifndef __kstring_hpp__
#define __kstring_hpp__

#include <string>
#include "kernel_allocator.hpp"

typedef std::basic_string<char, std::char_traits<char>, kernel_allocator<char> > kstring;

struct kstring_hash {
  inline size_t
  operator() (const kstring& s) const
  {
    const char* c = s.c_str ();
    size_t retval = 0;
    while (*c != 0) {
      retval *= 9348;
      retval += *c;
      ++c;
    }
    return retval;
  }
 
};

#endif
