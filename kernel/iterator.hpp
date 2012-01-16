#ifndef __iterator_hpp__
#define __iterator_hpp__

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag {};
struct bidirectional_iterator_tag {};
struct random_access_iterator_tag {};

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

#endif /* __iterator_hpp__ */
