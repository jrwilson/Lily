#ifndef __kstring_hpp__
#define __kstring_hpp__

#include <string>
#include "kernel_allocator.hpp"

typedef std::basic_string<char, std::char_traits<char>, kernel_allocator<char> > kstring;

struct kstring_hash {
  inline size_t
  operator() (const kstring& s) const
  {
    return std::hash<const char*> () (s.c_str ());
  }
 
};

#endif
