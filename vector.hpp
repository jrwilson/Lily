#ifndef __vector_hpp__
#define __vector_hpp__

/*
  File
  ----
  vector.hpp
  
  Description
  -----------
  A vector implementation.

  Authors:
  Justin R. Wilson
*/

#include "memory.hpp"
#include "algorithm.hpp"
#include "iterator.hpp"
#include "type_traits.hpp"

static const size_t INITIAL_CAPACITY = 8;

template <class T, class Allocator = allocator<T> >
class vector : private Allocator {
public:
  typedef T value_type;
  typedef Allocator allocator_type;
  typedef typename Allocator::size_type size_type;
  typedef typename Allocator::difference_type difference_type;
  typedef typename Allocator::reference reference;
  typedef typename Allocator::const_reference const_reference;
  typedef typename Allocator::pointer pointer;
  typedef typename Allocator::const_pointer const_pointer;
  typedef typename Allocator::pointer iterator;
  typedef typename Allocator::const_pointer const_iterator;
  typedef ::reverse_iterator<iterator> reverse_iterator;
  typedef ::reverse_iterator<const_iterator> const_reverse_iterator;

private:
  pointer begin_;
  pointer end_;
  pointer final_;

public:
  explicit vector (const Allocator& a = Allocator ()) :
    Allocator (a)
  {
    begin_ = Allocator::allocate (INITIAL_CAPACITY);
    end_ = begin_;
    final_ = begin_ + INITIAL_CAPACITY;
  }

  explicit vector (size_type n,
  		   const T& value = T (),
  		   const Allocator& a = Allocator ()) :
    Allocator (a)
  {
    construct_n (n, value);
  }

private:
  void construct_n (size_type n,
		    const T& value)
  {
    const size_type cap = max (n, INITIAL_CAPACITY);
    begin_ = allocate (cap);
    end_ = begin_ + n;
    final_ = begin_ + cap;

    pointer ptr = begin_;
    while (ptr != end_) {
      construct (ptr++, value);
    }
  }

  template <class InputIterator>
  void
  construct_dispatch (InputIterator n,
		      InputIterator value,
		      true_type)
  {
    construct_n (n, value);
  }

  template <class InputIterator>
  void
  construct_dispatch (InputIterator begin,
		      InputIterator end,
		      false_type)
  {
    const size_type cap = max (static_cast<size_type> (distance (begin, end)), INITIAL_CAPACITY);
    begin_ = allocate (cap);
    end_ = begin_;
    final_ = begin_ + cap;

    while (begin != end) {
      construct (end_++, *begin++);
    }
  }

public:
  template <class InputIterator>
  vector (InputIterator begin,
  	  InputIterator end,
  	  const Allocator& a = Allocator ()) :
    Allocator (a)
  {
    construct_dispatch (begin, end, typename is_integral<InputIterator>::type ());
  }

  vector (const vector<T, Allocator>& x) :
    Allocator (x)
  {
    const size_type cap = x.capacity ();
    begin_ = allocate (cap);
    end_ = begin_;
    final_ = begin_ + cap;

    pointer src = x.begin_;
    while (src != x.end_) {
      construct (end_++, *src++);
    }
  }

  ~vector ()
  {
    clear ();
    deallocate (begin_, capacity ());
  }

  vector<T, Allocator>&
  operator= (const vector<T, Allocator>& x)
  {
    if (this != &x) {
      clear ();
      reserve (x.capacity ());
      pointer src = x.begin_;
      while (src != x.end_) {
	construct (end_++, *src++);
      }
    }
    return *this;
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
    return reverse_iterator (end ());
  }

  const_reverse_iterator
  rbegin () const
  {
    return reverse_iterator (end ());
  }

  reverse_iterator
  rend ()
  {
    return reverse_iterator (begin ());
  }

  const_reverse_iterator
  rend () const
  {
    return reverse_iterator (begin ());
  }

  size_type
  size () const
  {
    return end_ - begin_;
  }

  size_type
  max_size () const
  {
    return Allocator::max_size ();
  }

  void
  resize (size_type sz,
	  const T& c = T ())
  {
    if (sz > size ()) {
      reserve (sz);
      pointer end = begin_ + sz;
      while (end_ != end) {
	construct (end_++, c);
      }
    }
    else if (sz < size ()) {
      pointer end = begin_ + sz;
      while (end_ != end) {
  	destroy (--end_);
      }
    }
  }

  size_type
  capacity () const
  {
    return final_ - begin_;
  }

  bool
  empty () const
  {
    return begin_ == end_;
  }

  void
  reserve (size_type n)
  {
    size_type old_cap = capacity ();
    if (n > old_cap) {
      n = max (n, 2 * old_cap);
      pointer new_begin = allocate (n);
      pointer new_end = new_begin;

      pointer src = begin_;
      while (src != end_) {
      	construct (new_end++, *src);
      	destroy (src++);
      }

      deallocate (begin_, old_cap);

      begin_ = new_begin;
      end_ = new_end;
      final_ = new_begin + n;
    }
  }

  reference
  operator[] (size_type n)
  {
    return begin_[n];
  }

  const_reference
  operator[] (size_type n) const
  {
    return begin_[n];
  }

  reference
  at (size_type n)
  {
    return begin_[n];
  }

  const_reference
  at (size_type n) const
  {
    return begin_[n];
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
  assign (size_type n,
	  const T& u)
  {
    assign_n (n, u);
  }

private:
  void
  assign_n (size_type n,
	    const T& u)
  {
    clear ();
    reserve (n);
    while (n != 0) {
      construct (end_++, u);
      --n;
    }
  }

  template <class InputIterator>
  void
  assign_dispatch (InputIterator n,
		   InputIterator u,
		   true_type)
  {
    assign_n (n, u);
  }

  template <class InputIterator>
  void
  assign_dispatch (InputIterator begin,
		   InputIterator end,
		   false_type)
  {
    clear ();
    reserve (distance (begin, end));
    while (begin != end) {
      construct (end_++, *begin++);
    }
  }
  
public:
  template <class InputIterator>
  void
  assign (InputIterator begin,
	  InputIterator end)
  {
    assign_dispatch (begin, end, typename is_integral<InputIterator>::type ());
  }

  void
  push_back (const T& x)
  {
    reserve (size () + 1);
    construct (end_++, x);
  }

  void
  pop_back ()
  {
    destroy (--end_);
  }

  iterator
  insert (iterator position,
	  const T& x)
  {
    return insert_n (position, 1, x);
  }

  void
  insert (iterator position,
	  size_type n,
	  const T& x)
  {
    insert_n (position, n, x);
  }

private:
  iterator
  insert_n (iterator position,
	    size_type n,
	    const T& x)
  {
    // Convert to an offset.
    difference_type offset = position - begin_;
    reserve (size () + n);
    // Recalculate the iterator.
    position = begin_ + offset;
    // Copy.
    pointer src = end_;
    pointer dst = end_ + n;
    while (src != position) {
      construct (--dst, *--src);
      destroy (src);
    }
    end_ += n;
    // Insert.
    while (n != 0) {
      construct (src++, x);
      --n;
    }
    return position;
  }

  template <class InputIterator>
  void
  insert_dispatch (iterator position,
		   InputIterator begin,
		   InputIterator end,
		   true_type)
  {
    insert_n (position, begin, end);
  }

  template <class InputIterator>
  void
  insert_dispatch (iterator position,
		   InputIterator begin,
		   InputIterator end,
		   false_type)
  {
    // Convert to offset.
    difference_type offset = position - begin_;
    size_type n = distance (begin, end);
    // Reserve space.
    reserve (size () + n);
    // Recalculate the iterator.
    position = begin_ + offset;
    // Copy.
    pointer src = end_;
    pointer dst = end_ + n;
    while (src != position) {
      construct (--dst, *--src);
      destroy (src);
    }
    end_ += n;
    // Insert.
    while (begin != end) {
      construct (position++, *begin++);
    }
  }  

public:

  template <class InputIterator>
  void
  insert (iterator position,
	  InputIterator begin,
	  InputIterator end)
  {
    insert_dispatch (position, begin, end, typename is_integral<InputIterator>::type ());
  }

  iterator
  erase (iterator position)
  {
    return erase (position, position + 1);
  }

  iterator
  erase (iterator begin,
	 iterator end)
  {
    iterator src = begin;
    // Destroy.
    while (src != end) {
      destroy (src++);
    }
    iterator dst = begin;
    while (src != end_) {
      construct (dst++, *src);
      destroy (src++);
    }
    end_ = dst;
    return begin;
  }

  void
  swap (vector& vec)
  {
    ::swap (begin_, vec.begin_);
    ::swap (end_, vec.end_);
    ::swap (final_, vec.final_);
  }

  void
  clear ()
  {
    while (end_ != begin_) {
      destroy (--end_);
    }
  }

  allocator_type
  get_allocator () const {
    return Allocator (*this);
  }
};

template <class T, class Allocator>
bool
operator== (const vector<T, Allocator>& x,
	    const vector<T, Allocator>& y)
{
  return (x.size () == y.size ()) && equal (x.begin (), x.end (), y.begin ());
}

template <class T, class Allocator>
bool
operator< (const vector<T, Allocator>& x,
	   const vector<T, Allocator>& y)
{
  return lexicographical_compare (x.begin (), x.end (), y.begin (), y.end ());
}

template <class T, class Allocator>
bool
operator!= (const vector<T, Allocator>& x,
	    const vector<T, Allocator>& y)
{
  return !(x == y);
}

template <class T, class Allocator>
bool
operator> (const vector<T, Allocator>& x,
	   const vector<T, Allocator>& y)
{
  return y < x;
}

template <class T, class Allocator>
bool
operator<= (const vector<T, Allocator>& x,
	    const vector<T, Allocator>& y)
{
  return !(y < x);
}

template <class T, class Allocator>
bool
operator>= (const vector<T, Allocator>& x,
	    const vector<T, Allocator>& y)
{
  return !(x < y);
}

template <class T, class Allocator>
void
swap (vector<T, Allocator>& x,
      vector<T, Allocator>& y)
{
  x.swap (y);
}

#endif /* __vector_hpp__ */
