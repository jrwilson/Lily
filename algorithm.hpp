#ifndef __algorithm_hpp__
#define __algorithm_hpp__

/*
  File
  ----
  algorithm.hpp
  
  Description
  -----------
  Algorithms.

  Authors:
  Justin R. Wilson
*/

template <class T>
const T& min (const T& x,
	      const T& y) {
  return (x < y) ? x : y;
}

template <class T>
const T& max (const T& x,
	      const T& y) {
  return (x < y) ? y : x;
}

#endif /* __algorithm_hpp_ */