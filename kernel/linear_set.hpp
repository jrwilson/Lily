#ifndef __linear_set_hpp__
#define __linear_set_hpp__

#include "lin_assoc_impl.hpp"
#include "identity_selector.hpp"

template <typename T,
	  typename Hash = hash<T>,
	  typename Equal = equal_to<T>,
	  typename Allocator = allocator<T> >
class linear_set : public lin_assoc_cont<T, T, identity_selector<T>, Hash, Equal, Allocator> {
private:
  typedef lin_assoc_cont<T, T, identity_selector<T>, Hash, Equal, Allocator> impl_type;
public:
  typedef typename impl_type::allocator_type allocator_type;

  typedef typename impl_type::hasher hasher;
  typedef typename impl_type::key_equal key_equal;

  typedef typename impl_type::key_type key_type;
  typedef typename impl_type::value_type value_type;
  typedef typename impl_type::pointer pointer;
  typedef typename impl_type::const_pointer const_pointer;
  typedef typename impl_type::reference reference;
  typedef typename impl_type::const_reference const_reference;
  typedef typename impl_type::size_type size_type;
  typedef typename impl_type::difference_type difference_type;

  typedef typename impl_type::local_iterator local_iterator;
  typedef typename impl_type::const_local_iterator const_local_iterator;

  typedef typename impl_type::iterator iterator;
  typedef typename impl_type::const_iterator const_iterator;
};

#endif /* __linear_set_hpp__ */
