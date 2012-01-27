#ifndef __vector_hpp__
#define __vector_hpp__

#include "memory.hpp"
#include "iterator.hpp"
#include "string.hpp"

template <typename T,
	  class Allocator = allocator<T> >
class vector : private Allocator {
private:
  T* begin_;
  T* end_;
  T* limit_;

public:
  typedef Allocator allocator_type;

  typedef typename Allocator::value_type value_type;
  typedef typename Allocator::pointer pointer;
  typedef typename Allocator::const_pointer const_pointer;
  typedef typename Allocator::reference reference;
  typedef typename Allocator::const_reference const_reference;
  typedef typename Allocator::size_type size_type;
  typedef typename Allocator::difference_type difference_type;

  typedef pointer iterator;
  typedef const_pointer const_iterator;
  typedef ::reverse_iterator<pointer> reverse_iterator;
  typedef ::reverse_iterator<const_pointer> const_reverse_iterator;

  explicit vector (const Allocator& = Allocator ())
  {
    begin_ = Allocator::allocate (2);
    end_ = begin_;
    limit_ = begin_ + 2;
  }

  explicit vector (size_type n,
		   const T& value = T (),
		   const Allocator& = Allocator ())
  {
    size_type count = max (n, size_type (2));
    begin_ = Allocator::allocate (count);
    limit_ = begin_ + count;
    for (end_ = begin_; n != 0; --n) {
      Allocator::construct (end_++, value);
    }
  }

  vector (const vector& other) :
    allocator_type (other)
  {
    size_type count = max (other.size (), size_type (2));
    begin_ = Allocator::allocate (count);
    end_ = begin_;
    limit_ = begin_ + count;
    for (T* p = other.begin_; p != other.end_; ++p) {
      Allocator::construct (end_++, *p);
    }
  }

  ~vector ()
  {
    for (T* ptr = begin_; ptr != end_; ++ptr) {
      Allocator::destroy (ptr);
    }
    Allocator::deallocate (begin_, capacity ());
  }

  size_type
  capacity () const
  {
    return limit_ - begin_;
  }

  size_type
  size () const
  {
    return end_ - begin_;
  }

  bool
  empty () const
  {
    return begin_ == end_;
  }

  iterator
  begin ()
  {
    return begin_;
  }
  
  const_iterator
  begin () const
  {
    return begin_;
  }

  iterator
  end ()
  {
    return end_;
  }
  
  const_iterator
  end () const
  {
    return end_;
  }

  reverse_iterator
  rbegin ()
  {
    return reverse_iterator (end_);
  }

  const_reverse_iterator
  rbegin () const
  {
    return const_reverse_iterator (end_);
  }

  reverse_iterator
  rend ()
  {
    return reverse_iterator (begin_);
  }

  const_reverse_iterator
  rend () const
  {
    return const_reverse_iterator (begin_);
  }

  reference
  operator[] (size_type idx)
  {
    return begin_[idx];
  }

  reference
  front ()
  {
    return *begin_;
  }

  const_reference
  front () const
  {
    return *begin_;
  }

  reference
  back ()
  {
    return *(end_ - 1);
  }

  const_reference
  back () const
  {
    return *(end_ - 1);
  }

  void
  reserve (size_type n)
  {
    const size_type old_capacity = capacity ();
    if (n > old_capacity) {
      const size_type new_capacity = max (n, 2 * old_capacity);
      T* new_begin = Allocator::allocate (new_capacity);
      T* new_end = new_begin + size ();
      T* new_limit = new_begin + new_capacity;

      memcpy (new_begin, begin_, size () * sizeof (T));
      Allocator::deallocate (begin_, old_capacity);

      begin_ = new_begin;
      end_ = new_end;
      limit_ = new_limit;
    }
  }

  void
  resize (size_type n,
	  const T& value = T ())
  {
    size_type old_size = size ();
    if (n > old_size) {
      reserve (n);
      for (; old_size != n; ++old_size) {
	Allocator::construct (end_++, value);
      }
    }
    else {
      for (; old_size != n; --old_size) {
	Allocator::destroy (--end_);
      }
    }
  }

  template <typename InputIterator>
  void
  insert (iterator pos,
  	  InputIterator begin,
  	  InputIterator end)
  {
    size_type idx = pos - begin_;
    size_type count = distance (begin, end);
    reserve (size () + count);
    pos = begin_ + idx;
    memmove (pos + count, pos, (end_ - pos) * sizeof (T));
    for (; begin != end; ++begin, ++pos) {
      Allocator::construct (pos, *begin);
    }
  }

  iterator
  insert (iterator pos,
	  const T& value)
  {
    // TODO:  If we are full, then we copy twice.  This could be reduces to once.
    size_type idx = pos - begin_;
    // This might invalidate the iterator.
    reserve (size () + 1);
    pos = begin_ + idx;
    memmove (pos + 1, pos, (end_ - pos) * sizeof (T));
    ++end_;
    Allocator::construct (pos, value);
    return pos;
  }

  iterator
  erase (iterator pos)
  {
    Allocator::destroy (pos);
    memmove (pos, pos + 1, (end_ - pos) * sizeof (T));
    --end_;
    return pos;
  }

  void
  push_back (const T& value)
  {
    reserve (size () + 1);
    Allocator::construct (end_++, value);
  }

  void
  clear ()
  {
    for (T* ptr = begin_; ptr != end_; ++ptr) {
      Allocator::destroy (ptr);
    }
    end_ = begin_;
  }

};

#endif /* __vector_hpp__ */
