#ifndef __slist_hpp__
#define __slist_hpp__

#include "memory.hpp"
#include "iterator.hpp"

template <typename T>
struct slist_node {
  slist_node* next;
  T value;
};

template <typename T,
	  class Allocator = allocator<T> >
class slist : private Allocator::template rebind<slist_node<T> >::other {
private:
  typedef slist_node<T> node_type;
  typedef typename Allocator::template rebind<node_type>::other node_allocator_type;

  template <typename NodeType,
	    typename ValueType,
	    typename Pointer,
	    typename Reference>
  class iter : public iterator<forward_iterator_tag, ValueType> {
  private:
    NodeType* n_;
  public:
    iter (NodeType* n) :
      n_ (n)
    { }

    inline bool
    operator == (const iter& other) const
    {
      return n_ == other.n_;
    }

    inline bool
    operator != (const iter& other) const
    {
      return n_ != other.n_;
    }
    
    Reference
    operator* () const
    {
      return n_->value;
    }

    Pointer
    operator-> () const
    {
      return &n_->value;
    }

    iter&
    operator++ ()
    {
      n_ = n_->next;
      return *this;
    }
  };

  node_type* n_;

public:
  typedef typename allocator<T>::value_type value_type;
  typedef typename allocator<T>::pointer pointer;
  typedef typename allocator<T>::const_pointer const_pointer;
  typedef typename allocator<T>::reference reference;
  typedef typename allocator<T>::const_reference const_reference;
  typedef typename allocator<T>::size_type size_type;
  typedef typename allocator<T>::difference_type difference_type;

  typedef iter<node_type, value_type, pointer, reference> iterator;
  typedef iter<const node_type, const value_type, const_pointer, const_reference> const_iterator;

  explicit slist (const Allocator& = Allocator ()) :
    n_ (0)
  { }

  iterator
  begin ()
  {
    return iterator (n_);
  }
  
  const_iterator
  begin () const
  {
    return const_iterator (n_);
  }

  iterator
  end ()
  {
    return iterator (0);
  }
  
  const_iterator
  end () const
  {
    return const_iterator (0);
  }

  void
  push_front (const T& value)
  {
    node_type* n = node_allocator_type::allocate (1);
    Allocator::construct (&n->value, value);
    n->next = n_;
    n_ = n;
  }

};

#endif /* __slist_hpp__ */
