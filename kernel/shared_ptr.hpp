#ifndef __shared_ptr_hpp__
#define __shared_ptr_hpp__

#include <stddef.h>

// From The C++ Standard Library:  A Tutorial and Reference by Josuttis pp. 222-223.

// TODO:  Make these operations atomic.
template <typename T>
class shared_ptr {
private:
  T* ptr;
  size_t* count;

public:
  explicit shared_ptr (T* p = 0) :
    ptr (p),
    count (new size_t (1))
  { }

  shared_ptr (const shared_ptr<T>& p) :
    ptr (p.ptr),
    count (p.count)
  {
    ++*count;
  }

  shared_ptr<T>&
  operator= (const shared_ptr<T>& p)
  {
    if (this != &p) {
      dispose ();
      ptr = p.ptr;
      count = p.count;
      ++*count;
    }
    return *this;
  }

  ~shared_ptr ()
  {
    dispose ();
  }

  inline T&
  operator* () const
  {
    return *ptr;
  }

  inline T*
  operator-> () const
  {
    return ptr;
  }

  inline T*
  get () const
  {
    return ptr;
  }

  inline bool
  operator== (const shared_ptr<T>& other) const
  {
    return ptr == other.ptr;
  }

  inline bool
  operator!= (const shared_ptr<T>& other) const
  {
    return ptr != other.ptr;
  }

private:
  inline void
  dispose () {
    if (--*count == 0) {
      delete ptr;
      delete count;
    }
  }
};

#endif /* __shared_ptr_hpp__ */
