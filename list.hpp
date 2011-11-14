#ifndef __list_hpp__
#define __list_hpp__

/*
  File
  ----
  list.hpp
  
  Description
  -----------
  Doubly-linked list.

  Authors:
  Justin R. Wilson
*/

template <class T, class A>
class list {
  struct Iterator;
  struct ConstIterator;

public:
  typedef A allocator_type;
  typedef typename A::pointer pointer;
  typedef typename A::const_pointer const_pointer;
  typedef typename A::reference reference;
  typedef typename A::const_reference const_reference;
  typedef typename A::value_type value_type;
  typedef Iterator iterator;
  typedef ConstIterator const_iterator;
  typedef typename A::size_type size_type;
  typedef typename A::difference_type difference_type;
  // typedef reverse_iterator<const_iterator> const_reverse_iterator;
  // typedef reverse_iterator<iterator> reverse_iterator;

  explicit list (const A& al) { }
  explicit list (size_type n,
		 const A& al);
  list (size_type n,
	const T& v,
	const A& al);
  list (const list& x);
  template <class In>
  list (In first,
	In last,
	const A& al);
  iterator begin ();
  const_iterator begin () const;
  iterator end ();
  const_iterator end () const;
  // reverse_iterator rbegin ();
  // const_reverse_iterator rbegin () const;
  // reverse_iterator rend ();
  // const_reverse_iterator rend () const;
  void resize (size_type n);
  void resize (size_type n,
	       T x);
  size_type size () const;
  size_type max_size () const;
  bool empty () const;
  A get_allocator () const;
  reference front ();
  const_reference front () const;
  reference back ();
  const_reference back () const;
  void push_front (const T& x);
  void pop_front ();
  void push_back (const T& x);
  void pop_back ();
  template<class In>
  void assign (In first,
	       In last);
  void assign (size_type n,
	       const T& x);
  iterator insert (iterator it,
		   const T& x);
  void insert (iterator it, size_type n, const T& x);
  template <class In>
  void insert (iterator it,
	       In first,
	       In last);
  iterator erase (iterator it);
  iterator erase (iterator first,
		  iterator last);
  void clear ();
  void swap (list& x);
  void splice (iterator it,
	       list& x);
  void splice (iterator it,
	       list& x,
	       iterator first);
  void splice (iterator it,
	       list& x,
	       iterator first,
	       iterator last);
  void remove (const T& x);
  template <class Pred>
  void remove_if (Pred pr);
  void unique ();
  template<class Pred>
  void unique (Pred pr);
  void merge (list& x);
  template <class Pred>
  void merge (list& x,
	      Pred pr);
  void sort ();
  template <class Pred>
  void sort (Pred pr);
  void reverse ();
};

template <class T, class A>
bool operator== (const list<T, A>& lhs,
		 const list<T, A>& rhs);

template <class T, class A>
bool operator!= (const list<T, A>& lhs,
		 const list<T, A>& rhs);

template <class T, class A>
bool operator< (const list<T, A>& lhs,
		const list<T, A>& rhs);

template <class T, class A>
bool operator> (const list<T, A>& lhs,
		const list<T, A>& rhs);

template <class T, class A>
bool operator<= (const list<T, A>& lhs,
		 const list<T, A>& rhs);

template <class T, class A>
bool operator>= (const list<T, A>& lhs,
		 const list<T, A>& rhs);

template <class T, class A>
void swap (list<T, A>& lhs,
	   list<T, A>& rhs);

#endif /* __list_hpp__ */
