#ifndef __shared_ptr_hpp__
#define __shared_ptr_hpp__

/* From The C++ Standard Library:  A Tutorial and Reference by Josuttis pp. 222-223. */

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

  ~shared_ptr () {
    dispose ();
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

  T&
  operator* () const
  {
    return *ptr;
  }

  T*
  operator-> () const
  {
    return ptr;
  }

private:
  void
  dispose ()
  {
    if (--*count == 0) {
      delete count;
      delete ptr;
    }
  }
};

#endif /* __shared_ptr_hpp__ */
