#ifndef __unordered_map_hpp__
#define __unordered_map_hpp__

#include "uno_assoc_impl.hpp"
#include "memory.hpp"

template <typename Key,
	  typename T>
struct first_selector {
  const Key&
  operator() (const pair<Key, T>& p) const
  {
    return p.first;
  }
};

template <typename Key,
	  typename T,
	  typename Hash = hash<Key>,
	  typename Equal = equal_to<Key>,
	  typename Allocator = allocator<pair<const Key, T> > >
class unordered_map : public uno_assoc_cont<Key, pair<const Key, T>, first_selector<const Key, T>, Hash, Equal, Allocator> {
private:
  typedef uno_assoc_cont<Key, pair<const Key, T>, first_selector<const Key, T>, Hash, Equal, Allocator> impl_type;
public:
  typedef typename impl_type::allocator_type allocator_type;

  typedef typename impl_type::hasher hasher;
  typedef typename impl_type::key_equal key_equal;

  typedef typename impl_type::key_type key_type;
  typedef typename impl_type::mapped_type mapped_type;
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

#endif /* __unordered_map_hpp__ */
