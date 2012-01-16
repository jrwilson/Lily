#ifndef __algorithm_hpp__
#define __algorithm_hpp__

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
	  typename T>
ForwardIterator
lower_bound (ForwardIterator begin,
	     ForwardIterator end,
	     const T& value)
{
  // TODO:  Implement binary search.
  for (; !(begin == end) && *begin < value; ++begin) ;;
  return begin;
}

template <typename T>
T
max (const T& x,
     const T& y)
{
  return (x < y) ? y : x;
}

template <typename T>
T
min (const T& x,
     const T& y)
{
  return (x < y) ? x : y;
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
