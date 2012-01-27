#ifndef __deque_hpp__
#define __deque_hpp__

#include "memory.hpp"
#include "iterator.hpp"

template <typename T,
	  class Allocator = allocator<T> >
class deque : private Allocator {
private:
  typedef deque<T, Allocator> deque_type;

  T* start_;
  T* begin_;
  T* end_;
  T* limit_;

  void
  advance (T*& ptr) const
  {
    ++ptr;
    if (ptr == limit_) {
      ptr = start_;
    }
  }

  deque (const deque&) { }

public:
  typedef Allocator allocator_type;

  typedef typename Allocator::value_type value_type;
  typedef typename Allocator::pointer pointer;
  typedef typename Allocator::const_pointer const_pointer;
  typedef typename Allocator::reference reference;
  typedef typename Allocator::const_reference const_reference;
  typedef typename Allocator::size_type size_type;
  typedef typename Allocator::difference_type difference_type;

  explicit deque (const Allocator& = Allocator ())
  {
    start_ = Allocator::allocate (2);
    begin_ = start_;
    end_ = start_;
    limit_ = start_ + 2;
  }

  ~deque ()
  {
    for (T* ptr = begin_; ptr != end_; advance (ptr)) {
      Allocator::destroy (ptr);
    }
    Allocator::deallocate (start_, limit_ - start_);
  }

  size_type
  capacity () const
  {
    // Leave one open to determine when full.
    return limit_ - start_ - 1;
  }

  size_type
  size () const
  {
    if (begin_ > end_) {
      return end_ + (limit_ - start_) - begin_;
    }
    else {
      return end_ - begin_;
    }
  }

  bool
  empty () const
  {
    return begin_ == end_;
  }

  const_reference
  front () const
  {
    return *begin_;
  }

  void
  reserve (size_type n)
  {
    const size_type old_capacity = capacity ();
    if (n > old_capacity) {
      const size_type new_capacity = max (n, 2 * old_capacity);
      T* new_start = Allocator::allocate (new_capacity + 1);
      T* new_begin = new_start;
      T* new_end = new_begin + size ();
      T* new_limit = new_start + new_capacity + 1;

      if (begin_ < end_) {
	memcpy (new_begin, begin_, size () * sizeof (T));
      }
      else {
	memcpy (new_begin, begin_, (limit_ - begin_) * sizeof (T));
	memcpy (new_begin + (limit_ - begin_), start_, (end_ - start_) * sizeof (T));
      }
      Allocator::deallocate (start_, limit_ - start_);
      start_ = new_start;
      begin_ = new_begin;
      end_ = new_end;
      limit_ = new_limit;
    }
  }

  void
  pop_front ()
  {
    Allocator::destroy (begin_);
    advance (begin_);
  }

  void
  push_back (const T& value)
  {
    reserve (size () + 1);
    Allocator::construct (end_, value);
    advance (end_);
  }
};

#endif /* __deque_hpp__ */
