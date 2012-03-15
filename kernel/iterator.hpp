#ifndef __iterator_hpp__
#define __iterator_hpp__

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag, public output_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

template <typename Category,
	  typename T,
	  typename Difference = ptrdiff_t,
	  class Pointer = T*,
	  class Reference = T&>
struct iterator {
  typedef T value_type;
  typedef Difference difference_type;
  typedef Pointer pointer;
  typedef Reference reference;
  typedef Category iterator_category;
};

template <typename Iterator>
struct iterator_traits {
  typedef typename Iterator::value_type value_type;
  typedef typename Iterator::difference_type difference_type;
  typedef typename Iterator::pointer pointer;
  typedef typename Iterator::reference reference;
  typedef typename Iterator::iterator_category iterator_category;
};

template <typename T>
struct iterator_traits<T*> {
  typedef T value_type;
  typedef ptrdiff_t difference_type;
  typedef T* pointer;
  typedef T& reference;
  typedef random_access_iterator_tag iterator_category;
};

template <typename T>
struct iterator_traits<const T*> {
  typedef T value_type;
  typedef ptrdiff_t difference_type;
  typedef const T* pointer;
  typedef const T& reference;
  typedef random_access_iterator_tag iterator_category;
};

template <typename Iterator>
class reverse_iterator :
  public iterator<typename iterator_traits<Iterator>::iterator_category,
		  typename iterator_traits<Iterator>::value_type,
		  typename iterator_traits<Iterator>::difference_type,
		  typename iterator_traits<Iterator>::pointer,
		  typename iterator_traits<Iterator>::reference> {
private:
  Iterator base_;

public:
  typedef Iterator iterator_type;
  typedef typename iterator_traits<Iterator>::difference_type difference_type;
  typedef typename iterator_traits<Iterator>::pointer pointer;
  typedef typename iterator_traits<Iterator>::reference reference;

  reverse_iterator () { }

  explicit reverse_iterator (Iterator b) :
    base_ (b)
  { }

  template <typename U>
  reverse_iterator (const reverse_iterator<U>& other) :
    base_ (other.base ())
  { }

  Iterator
  base () const
  {
    return base_;
  }

  reference
  operator* () const
  {
    Iterator tmp = base_;
    return *(--tmp);
  }

  reverse_iterator
  operator+ (difference_type n) const
  {
    return reverse_iterator (base_ - n);
  }

  reverse_iterator&
  operator++ ()
  {
    --base_;
    return *this;
  }

  reverse_iterator
  operator++ (int) const
  {
    reverse_iterator retval = *this;
    --base_;
    return retval;
  }

  reverse_iterator&
  operator+= (difference_type n)
  {
    base_ -= n;
    return *this;
  }
  
  reverse_iterator
  operator- (difference_type n) const
  {
    return reverse_iterator (base_ + n);
  }

  reverse_iterator&
  operator-- ()
  {
    ++base_;
    return *this;
  }

  reverse_iterator
  operator-- (int) const
  {
    reverse_iterator retval = *this;
    ++base_;
    return retval;
  }

  reverse_iterator&
  operator-= (difference_type n)
  {
    base_ += n;
    return *this;
  }

};

template <typename T>
bool
operator== (const reverse_iterator<T>& iter1,
	    const reverse_iterator<T>& iter2)
{
  return iter1.base () == iter2.base ();
}

template <typename T>
bool
operator!= (const reverse_iterator<T>& iter1,
	    const reverse_iterator<T>& iter2)
{
  return !(iter1.base () == iter2.base ());
}

template <typename T1,
	  typename T2>
bool
operator== (const reverse_iterator<T1>& iter1,
	    const reverse_iterator<T2>& iter2)
{
  return iter1.base () == iter2.base ();
}

template <typename T1,
	  typename T2>
bool
operator!= (const reverse_iterator<T1>& iter1,
	    const reverse_iterator<T2>& iter2)
{
  return !(iter1.base () == iter2.base ());
}

template <typename InputIterator,
	  typename Distance>
void
advance_impl (InputIterator& iter,
	      Distance n,
	      random_access_iterator_tag)
{
  iter += n;
}

template <typename InputIterator,
	  typename Distance>
void
advance (InputIterator& iter,
	 Distance n)
{
  advance_impl (iter, n, typename iterator_traits<InputIterator>::iterator_category ());
}

template <typename InputIterator>
typename iterator_traits<InputIterator>::difference_type
distance_impl (InputIterator begin,
	       InputIterator end,
	       random_access_iterator_tag)
{
  return end - begin;
}

template <typename InputIterator>
typename iterator_traits<InputIterator>::difference_type
distance (InputIterator begin,
	  InputIterator end)
{
  return distance_impl (begin, end, typename iterator_traits<InputIterator>::iterator_category ());
}

/* From www.cplusplus.com */
template <typename T>
class back_insert_iterator :
  public iterator<output_iterator_tag, void, void, void, void> {
protected:
  T& t_;

public:
  typedef T container_type;

  explicit back_insert_iterator (T& t) :
    t_ (t)
  { }

  back_insert_iterator<T>&
  operator= (typename T::const_reference value)
  {
    t_.push_back (value);
    return *this;
  }

  back_insert_iterator<T>&
  operator* ()
  {
    return *this;
  }

  back_insert_iterator<T>&
  operator++ ()
  {
    return *this;
  }
  
  back_insert_iterator<T>
  operator++ (int)
  {
    return *this;
  }
};

template <typename T>
back_insert_iterator<T>
back_inserter (T& t)
{
  return back_insert_iterator<T> (t);
}

#endif /* __iterator_hpp__ */
