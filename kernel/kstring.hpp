#ifndef __kstring_hpp__
#define __kstring_hpp__

#include "kassert.hpp"
#include "string.hpp"

class kstring {
private:
  char* string_;
  size_t size_;

public:
  kstring () :
    string_ (0),
    size_ (0)
  { }

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

  void
  clear ()
  {
    delete[] string_;
    string_ = 0;
    size_ = 0;
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

  void
  append (const void* ptr,
	  size_t size)
  {
    char* str = new char[size_ + size];
    memcpy (str, string_, size_);
    memcpy (str + size_, ptr, size);
    delete[] string_;
    string_ = str;
    size_ += size;
  }
};

struct kstring_hash {
  size_t
  operator() (const kstring& str) const
  {
    size_t retval = 0;
    const char* ptr = str.c_str ();
    for (size_t i = 0; i != str.size (); ++i, ++ptr) {
      retval += *ptr;
      retval *= 1142821ul;
    }

    return retval;
  }
};

#endif /* __kstring_hpp__ */
