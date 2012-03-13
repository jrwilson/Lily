#ifndef __shared_ptr_hpp__
#define __shared_ptr_hpp__

// TODO:  These operations need to be atomic.

template <typename T>
class shared_ptr {
  //private:
public:
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

  T*
  get () const
  {
    return ptr;
  }

  bool
  operator== (const shared_ptr<T>& other) const
  {
    return ptr == other.ptr;
  }

private:
  void
  dispose () {
    if (--*count == 0) {
      delete ptr;
      delete count;
    }
  }
};

#endif /* __shared_ptr_hpp__ */
