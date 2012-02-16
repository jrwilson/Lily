#ifndef __kstring_hpp__
#define __kstring_hpp__

#include "kassert.hpp"
#include "string.hpp"

class kstring {
private:
  char* string_;
  size_t size_;

public:
  kstring (const char* str)
  {
    size_ = strlen (str) + 1;
    string_ = new char[size_];
    memcpy (string_, str, size_);
  }

  kstring (const char* str,
	   size_t size) :
    size_ (size)
  {
    string_ = new char[size_];
    memcpy (string_, str, size_);
  }

  kstring (const kstring& other)
  {
    size_ = other.size_;
    string_ = new char[size_];
    memcpy (string_, other.string_, size_);
  }

  kstring&
  operator= (const kstring& other)
  {
    if (this != &other) {
      delete[] string_;
      size_ = other.size_;
      string_ = new char[size_];
      memcpy (string_, other.string_, size_);
    }
    return *this;
  }

  ~kstring ()
  {
    delete[] string_;
  }

  size_t
  size () const
  {
    return size_;
  }

  const char*
  c_str () const
  {
    return string_;
  }

  bool
  operator== (const kstring& other) const
  {
    return size_ == other.size_ && memcmp (string_, other.string_, size_) == 0;
  }

  bool
  operator!= (const kstring& other) const
  {
    return !(*this == other);
  }
};

struct kstring_hash {
  size_t
  operator() (const kstring& str) const
  {
    size_t retval = 0;
    for (const char* begin = str.c_str ();
	 *begin != 0;
	 ++begin) {
      retval += *begin;
      retval *= 1142821ul;
    }

    return retval;
  }
};

#endif /* __kstring_hpp__ */
