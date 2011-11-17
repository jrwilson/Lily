#ifndef __iterator_hpp__
#define __iterator_hpp__

/*
  File
  ----
  iterator.hpp
  
  Description
  -----------
  Templates for iterators.

  Authors:
  Justin R. Wilson
*/

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag, output_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

template <class Iterator>
struct iterator_traits;

template <class T>
struct iterator_traits<T*> {
  typedef ptrdiff_t difference_type;
  typedef T value_type;
  typedef T* pointer;
  typedef T& reference;
  typedef random_access_iterator_tag iterator_category;
};

template <class T>
struct iterator_traits<const T*> {
  typedef ptrdiff_t difference_type;
  typedef T value_type;
  typedef const T* pointer;
  typedef const T& reference;
  typedef random_access_iterator_tag iterator_category;
};

template <class Iterator>
struct iterator_traits {
  typedef typename Iterator::difference_type difference_type;
  typedef typename Iterator::value_type value_type;
  typedef typename Iterator::pointer pointer;
  typedef typename Iterator::reference reference;
  typedef typename Iterator::iterator_category iterator_category;
};

template <class Category, class T, class Difference = ptrdiff_t, class Pointer = T*, class Reference = T&>
struct iterator {
  typedef Category iterator_category;
  typedef T value_type;
  typedef Difference difference_type;
  typedef Pointer pointer;
  typedef Reference reference;
};

template <class Iterator>
class reverse_iterator : public iterator<typename iterator_traits<Iterator>::iterator_category,
					 typename iterator_traits<Iterator>::value_type,
					 typename iterator_traits<Iterator>::difference_type,
					 typename iterator_traits<Iterator>::pointer,
					 typename iterator_traits<Iterator>::reference> {
protected:
  Iterator current;

public:
  typedef Iterator iterator_type;
  typedef typename iterator_traits<Iterator>::value_type value_type;
  typedef typename iterator_traits<Iterator>::difference_type difference_type;
  typedef typename iterator_traits<Iterator>::pointer pointer;
  typedef typename iterator_traits<Iterator>::reference reference;

  reverse_iterator () :
    current ()
  { }

  explicit reverse_iterator (Iterator x) :
    current (x)
  { }

  template <class U>
  reverse_iterator (const reverse_iterator<U>& x) :
    current (x.base ())
  { }

  Iterator base () const
  {
    return current;
  }

  reference operator* () const
  {
    Iterator tmp = current;
    return *--tmp;
  }

  pointer operator-> () const
  {
    Iterator tmp = current;
    return --tmp;
  }

  reference operator[] (difference_type n) const
  {
    Iterator tmp = current;
    return *(--tmp - n);
  }

  reverse_iterator&
  operator++ ()
  {
    --current;
    return *this;
  }

  reverse_iterator
  operator++ (int)
  {
    reverse_iterator tmp (current);
    --current;
    return tmp;
  }

  reverse_iterator&
  operator-- ()
  {
    ++current;
    return *this;
  }

  reverse_iterator
  operator-- (int)
  {
    reverse_iterator tmp (current);
    ++current;
    return tmp;
  }

  reverse_iterator
  operator+ (difference_type n) const
  {
    return reverse_iterator (current - n);
  }

  reverse_iterator&
  operator+= (difference_type n)
  {
    current -= n;
    return *this;
  }

  reverse_iterator
  operator- (difference_type n) const
  {
    return reverse_iterator (current + n);
  }

  reverse_iterator&
  operator-= (difference_type n)
  {
    current += n;
    return *this;
  }

};

template <class Iterator>
bool
operator== (const reverse_iterator<Iterator>& x,
	    const reverse_iterator<Iterator>& y)
{
  return x.base () == y.base ();
}

template <class Iterator>
bool
operator< (const reverse_iterator<Iterator>& x,
	   const reverse_iterator<Iterator>& y)
{
  return x.base () < y.base ();
}

template <class Iterator>
bool
operator!= (const reverse_iterator<Iterator>& x,
	    const reverse_iterator<Iterator>& y)
{
  return !(x == y);
}

template <class Iterator>
bool
operator> (const reverse_iterator<Iterator>& x,
	   const reverse_iterator<Iterator>& y)
{
  return y < x;
}

template <class Iterator>
bool
operator<= (const reverse_iterator<Iterator>& x,
	    const reverse_iterator<Iterator>& y)
{
  return !(y < x);
}

template <class Iterator>
bool
operator>= (const reverse_iterator<Iterator>& x,
	    const reverse_iterator<Iterator>& y)
{
  return !(x < y);
}
 
template <class Iterator>
typename reverse_iterator<Iterator>::difference_type
operator- (const reverse_iterator<Iterator>& x,
	   const reverse_iterator<Iterator>& y)
{
  return y.base () - x.base ();
}

template <class Iterator>
reverse_iterator<Iterator>
operator+ (typename reverse_iterator<Iterator>::difference_type n,
	   const reverse_iterator<Iterator>& x)
{
  return reverse_iterator<Iterator> (x.base () - n);
}

template <class IteratorX, class IteratorY>
bool
operator== (const reverse_iterator<IteratorX>& x,
	    const reverse_iterator<IteratorY>& y)
{
  return x.base () == y.base ();
}

template <class IteratorX, class IteratorY>
bool
operator< (const reverse_iterator<IteratorX>& x,
	   const reverse_iterator<IteratorY>& y)
{
  return x.base () < y.base ();
}

template <class IteratorX, class IteratorY>
bool
operator!= (const reverse_iterator<IteratorX>& x,
	    const reverse_iterator<IteratorY>& y)
{
  return !(x == y);
}

template <class IteratorX, class IteratorY>
bool
operator> (const reverse_iterator<IteratorX>& x,
	   const reverse_iterator<IteratorY>& y)
{
  return y < x;
}

template <class IteratorX, class IteratorY>
bool
operator<= (const reverse_iterator<IteratorX>& x,
	    const reverse_iterator<IteratorY>& y)
{
  return !(y < x);
}

template <class IteratorX, class IteratorY>
bool
operator>= (const reverse_iterator<IteratorX>& x,
	    const reverse_iterator<IteratorY>& y)
{
  return !(x < y);
}
 
template <class IteratorX, class IteratorY>
typename reverse_iterator<IteratorX>::difference_type
operator- (const reverse_iterator<IteratorX>& x,
	   const reverse_iterator<IteratorY>& y)
{
  return y.base () - x.base ();
}

template <class InputIterator>
typename iterator_traits<InputIterator>::difference_type
dist (InputIterator begin,
      InputIterator end,
      random_access_iterator_tag)
{
  return end - begin;
}

template <class InputIterator>
typename iterator_traits<InputIterator>::difference_type
dist (InputIterator begin,
      InputIterator end,
      input_iterator_tag)
{
  typename iterator_traits<InputIterator>::difference_type x = 0;
  for (; begin != end; ++begin, ++x) ;;
  return x;
}

template <class InputIterator>
typename iterator_traits<InputIterator>::difference_type
distance (InputIterator begin,
	  InputIterator end)
{
  return dist (begin, end, typename iterator_traits<InputIterator>::iterator_category ());
}

#endif /* __iterator_hpp_ */
