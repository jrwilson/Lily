#ifndef __algorithm_hpp__
#define __algorithm_hpp__

#include "iterator.hpp"
#include "functional.hpp"

template <typename InputIterator,
	  typename T>
InputIterator
find (InputIterator begin,
      InputIterator end,
      const T& value)
{
  for (; !(begin == end) && !(*begin == value); ++begin) ;;
  return begin;
}

template <typename InputIterator,
	  typename Predicate>
InputIterator
find_if (InputIterator begin,
	 InputIterator end,
	 Predicate predicate)
{
  for (; !(begin == end) && !predicate (*begin); ++begin) ;;
  return begin;
}

template <typename ForwardIterator,
	  typename T,
	  typename Predicate>
ForwardIterator
lower_bound (ForwardIterator begin,
	     ForwardIterator end,
	     const T& value,
	     Predicate predicate)
{
  typename iterator_traits<ForwardIterator>::difference_type count = distance (begin, end);
  while (count > 0) {
    ForwardIterator probe = begin;
    const typename iterator_traits<ForwardIterator>::difference_type step = count / 2;
    advance (probe, step);
    if (predicate (*probe, value)) {
      // Search the high-half.
      begin = ++probe;
      count -= step + 1;
    }
    else {
      // Search the low_half.
      count = step;
    }
  }
  return begin;
}

template <typename ForwardIterator,
	  typename T>
ForwardIterator
lower_bound (ForwardIterator begin,
	     ForwardIterator end,
	     const T& value)
{
  return lower_bound (begin, end, value, less<T> ());
}

template <typename ForwardIterator,
	  typename T,
	  typename Predicate>
ForwardIterator
upper_bound (ForwardIterator begin,
	     ForwardIterator end,
	     const T& value,
	     Predicate predicate)
{
  typename iterator_traits<ForwardIterator>::difference_type count = distance (begin, end);
  while (count > 0) {
    ForwardIterator probe = begin;
    const typename iterator_traits<ForwardIterator>::difference_type step = count / 2;
    advance (probe, step);
    if (!predicate (value, *probe)) {
      // Search the high-half.
      begin = ++probe;
      count -= step + 1;
    }
    else {
      // Search the low_half.
      count = step;
    }
  }
  return begin;
}

template <typename ForwardIterator,
	  typename T>
ForwardIterator
upper_bound (ForwardIterator begin,
	     ForwardIterator end,
	     const T& value)
{
  return upper_bound (begin, end, value, less<T> ());
}

template <typename T>
T
min (const T& x,
     const T& y)
{
  return (x < y) ? x : y;
}

template <typename T>
T
max (const T& x,
     const T& y)
{
  return (x < y) ? y : x;
}

template <typename RandomAccessIterator,
	  typename Predicate>
void sort (RandomAccessIterator begin,
	   RandomAccessIterator end,
	   Predicate pred)
{
  for (RandomAccessIterator marker = begin; marker != end; ++marker) {
    for (RandomAccessIterator cur = marker; cur != begin && !pred (* (cur - 1), *cur); --cur) {
      swap (*(cur - 1), *cur);
    }
  }
}

template <typename T>
void
swap (T& x,
      T& y)
{
  T temp = x;
  x = y;
  y = temp;
}

#endif /* __algorithm_hpp__ */
