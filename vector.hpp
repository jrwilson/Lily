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
  // typedef typename Allocator::const_pointer iterator;
  // typedef typename Allocator::const_pointer const_iterator;
  // typedef typename Allocator::const_pointer reverse_iterator;
  // typedef typename Allocator::const_pointer const_reverse_iterator;

private:
  static const size_type INITIAL_CAPACITY = 8;
  size_type size_;
  size_type capacity_;
  pointer data_;

public:
  explicit vector (const Allocator& a = Allocator ()) :
    Allocator (a),
    size_ (0),
    capacity_ (INITIAL_CAPACITY)
  {
    data_ = allocate (capacity_, 0);
  }

  // explicit vector (size_type n,
  // 		   const T& value = T (),
  // 		   const Allocator& = Allocator ());
  // template <class InputIterator>
  // vector (InputIterator first,
  // 	  InputIterator last,
  // 	  const Allocator& = Allocator ());
  // vector (const vector<T, Allocator>& x);

  // dtor
  // operator=

  // begin
  // end
  // rbegin
  // rend

  size_type size () const { return size_; }
  size_type capacity () const { return capacity_; }
  size_type max_size () const { return Allocator::max_size (); }
  bool empty () const { return size_ == 0; }
  void resize (size_type sz,
	       T c = T ()) {
    if (sz > size_) {
      if (sz > capacity_) {
	reserve (sz);
	while (size_ < sz) {
	  construct (data_ + size_++, c);
	}
      }
    }
    else if (sz < size_) {
      while (sz < size_) {
	destroy (data_ + --size_);
      }
    }
  }
  void reserve (size_type n) {
    if (n > capacity_) {
      pointer new_data = allocate (n);
      for (size_type idx = 0; idx < size_; ++idx) {
	construct (new_data + idx, data_[idx]);
      }
      deallocate (data_, capacity_);
      data_ = new_data;
      capacity_ = n;
    }
  }

  // operator[]
  // at
  // front
  // back

  // assign
  // push_back
  // pop_back
  // insert
  // erase
  // swap
  // clear

  // allocator_type
  // get_allocator () const {
  //   return 
  // }
};

#endif /* __vector_hpp__ */
