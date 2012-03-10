#ifndef __lin_assoc_impl_hpp__
#define __lin_assoc_impl_hpp__

/* Linear Associative Container Implementation for linear_set and linear_map.
   This data structure implements a set with a linear ordering.
 */

#include "functional.hpp"
#include "utility.hpp"

template <typename Value>
struct lin_assoc_bucket {
  Value value;
  size_t hash;
  lin_assoc_bucket* hash_next;
  lin_assoc_bucket* list_next;
  lin_assoc_bucket* list_prev;
};

template <typename Key,
	  typename Value,
	  typename Selector,
	  typename Hash,
	  typename Equal,
	  typename Allocator>
class lin_assoc_cont : public Allocator::template rebind<lin_assoc_bucket<Value> >::other,
		       public Allocator::template rebind<lin_assoc_bucket<Value*> >::other,
		       public Selector,
		       public Hash,
		       public Equal {
private:
  typedef lin_assoc_cont<Key, Value, Selector, Hash, Equal, Allocator> cont_type;
  typedef lin_assoc_bucket<Value> bucket_type;
  typedef typename Allocator::template rebind<bucket_type>::other bucket_allocator;
  typedef typename Allocator::template rebind<bucket_type*>::other lookup_allocator;

public:
  typedef Allocator allocator_type;

  typedef Key key_type;
  typedef Value mapped_type;
  typedef typename Allocator::value_type value_type;
  typedef typename Allocator::pointer pointer;
  typedef typename Allocator::const_pointer const_pointer;
  typedef typename Allocator::reference reference;
  typedef typename Allocator::const_reference const_reference;
  typedef typename Allocator::size_type size_type;
  typedef typename Allocator::difference_type difference_type;

  struct local_iterator { };
  struct const_local_iterator { };

  struct iterator {
    const cont_type* map_;
    bucket_type* bucket_;

    iterator (const cont_type* map,
  	      bucket_type* bucket) :
      map_ (map),
      bucket_ (bucket)
    { }
    
    bool
    operator== (const iterator& other) const
    {
      return bucket_ == other.bucket_;
    }

    bool
    operator!= (const iterator& other) const
    {
      return bucket_ != other.bucket_;
    }

    pointer
    operator-> () const
    {
      return &bucket_->value;
    }
  };

  struct const_iterator {
    const cont_type* map_;
    const bucket_type* bucket_;

    const_iterator () :
      map_ (0),
      bucket_ (0)
    { }

    const_iterator (const cont_type* map,
  		    const bucket_type* bucket) :
      map_ (map),
      bucket_ (bucket)
    { }

    const_iterator (const iterator& iter) :
      map_ (iter.map_),
      bucket_ (iter.bucket_)
    { }
    
    bool
    operator== (const const_iterator& other) const
    {
      return bucket_ == other.bucket_;
    }

    bool
    operator!= (const const_iterator& other) const
    {
      return bucket_ != other.bucket_;
    }

    const_pointer
    operator-> () const
    {
      return &bucket_->value;
    }

    const_iterator&
    operator++ ()
    {
      bucket_ = bucket_->list_next;
      return *this;
    }
  };

  typedef Hash hasher;
  typedef Equal key_equal;

private:

  bucket_type** lookup_;
  size_type lookup_size_;
  size_type size_;
  bucket_type* front_;
  bucket_type* back_;

  lin_assoc_cont (const lin_assoc_cont& other);

  // lin_assoc_cont (const lin_assoc_cont& other) :
  //   bucket_allocator (other),
  //   Allocator::template rebind<lin_assoc_bucket<Value*> >::other (other)
  // {
//     lookup_ = lookup_allocator::allocate (other.lookup_size_);
//     lookup_size_ = other.lookup_size_;
//     size_ = other.size_;
//     for (size_type idx = 0; idx != lookup_size_; ++idx) {
//       lookup_[idx] = 0;

//       for (bucket_type* ptr = other.lookup_[idx]; ptr != 0; ptr = ptr->next) {
// 	bucket_type* bucket = bucket_allocator::allocate (1);
// 	Allocator::construct (&bucket->value, ptr->value);
// 	bucket->hash = ptr->hash;
// 	bucket->next = lookup_[idx];
// 	lookup_[idx] = bucket;
//       }
//     }
  // }

public:
  lin_assoc_cont ()
  {
    lookup_ = lookup_allocator::allocate (2);
    lookup_size_ = 2;
    size_ = 0;
    front_ = 0;
    back_ = 0;
    for (size_type idx = 0; idx != lookup_size_; ++idx) {
      lookup_[idx] = 0;
    }
  }

  void
  clear ()
  {
    bucket_type* ptr = front_;
    while (ptr != 0) {
      bucket_type* tmp = ptr->list_next;
      ptr = tmp->list_next;
      Allocator::destroy (&tmp->value);
      bucket_allocator::deallocate (tmp, 1);
    }
    for (size_type idx = 0; idx < lookup_size_; ++idx) {
      lookup_[idx] = 0;
    }
    size_ = 0;
    front_ = 0;
    back_ = 0;
  }

  ~lin_assoc_cont ()
  {
    clear ();
    lookup_allocator::deallocate (lookup_, lookup_size_);
  }

  size_type
  size () const
  {
    return size_;
  }

  bool
  empty () const
  {
    return size_ == 0;
  }

  const reference
  front () const
  {
    return front_->value;
  }

  const reference
  back () const
  {
    return back_->value;
  }

  iterator
  begin ()
  {
    return iterator (this, front_);
  }

  const_iterator
  begin () const
  {
    return iterator (this, front_);
  }

  iterator
  end ()
  {
    return iterator (this, 0);
  }

  const_iterator
  end () const
  {
    return const_iterator (this, 0);
  }

private:
  bucket_type* find_ (const key_type& key) const
  {
    // Compute the hash.
    size_t hash = Hash::operator() (key);
    // Compute the key into the lookup table.
    size_type lookup_key = hash % lookup_size_;
    // Begin a search.
    for (bucket_type* bucket = lookup_[lookup_key];
	 bucket != 0;
	 bucket = bucket->hash_next) {
      if (bucket->hash == hash && Equal::operator() (Selector::operator () (bucket->value), key)) {
	// Found it.
	return bucket;
      }
    }

    return 0;
  }

public:

  iterator
  find (const key_type& key)
  {
    bucket_type* bucket = find_ (key);
    if (bucket != 0) {
      return iterator (this, bucket);
    }
    else {
      return end ();
    }
  }

  const_iterator
  find (const key_type& key) const
  {
    bucket_type* bucket = find_ (key);
    if (bucket != 0) {
      return const_iterator (this, bucket);
    }
    else {
      return end ();
    }
  }

  void
  pop_front ()
  {
    // First, take it out of the hash.

    // Find the bucket.
    // Compute the key into the lookup table.
    size_type lookup_key = front_->hash % lookup_size_;
    // Begin a search.
    for (bucket_type** bucket = &(lookup_[lookup_key]);
	 *bucket != 0;
	 bucket = &((*bucket)->hash_next)) {
      if (*bucket == front_) {
	// Remove it from the hash.
	bucket_type* tmp = *bucket;
	*bucket = tmp->hash_next;

	// Remove it from the linked list.
	bucket_type* prev = tmp->list_prev;
	bucket_type* next = tmp->list_next;

	if (prev != 0) {
	  prev->list_next = next;
	}
	else {
	  // We were the front.
	  front_ = next;
	}

	if (next != 0) {
	  next->list_prev = prev;
	}
	else {
	  // We were the back.
	  back_ = prev;
	}
	
	Allocator::destroy (&tmp->value);
	bucket_allocator::deallocate (tmp, 1);
	--size_;
	return;
      }
    }
  }

  bool
  push_back (const value_type& value)
  {
    // First, insert into the hash.

    // Compute the hash.
    size_t hash = Hash::operator() (Selector::operator () (value));
    // Compute the key into the lookup table.
    size_type lookup_key = hash % lookup_size_;
    // Begin a search.
    for (bucket_type* bucket = lookup_[lookup_key];
	 bucket != 0;
	 bucket = bucket->hash_next) {
      if (bucket->hash == hash && Equal::operator() (Selector::operator () (bucket->value), Selector::operator () (value))) {
	// Found it.  Do nothing.
	return false;
      }
    }
    
    bucket_type* bucket = bucket_allocator::allocate (1);
    Allocator::construct (&bucket->value, value);
    ++size_;
    bucket->hash = hash;
    bucket->hash_next = lookup_[lookup_key];
    lookup_[lookup_key] = bucket;

    // Second, insert into the list.
    if (back_ != 0) {
      back_->list_next = bucket;
      bucket->list_prev = back_;
      bucket->list_next = 0;
      back_ = bucket;
    }
    else {
      bucket->list_next = 0;
      bucket->list_prev = 0;
      front_ = bucket;
      back_ = bucket;
    }
    
    // Rehash when size_ / lookup_size_ > .80
    // Rehash when size_ / lookup_size_ > 80 / 100
    // Rehash when 100 * size_ > 80 * lookup_size_
    if (100 * size_ > 80 * lookup_size_) {
      size_type new_lookup_size = 2 * lookup_size_;
      bucket_type** new_lookup = lookup_allocator::allocate (new_lookup_size);
      for (size_type idx = 0; idx < new_lookup_size; ++idx) {
    	new_lookup[idx] = 0;
      }

      for (size_type idx = 0; idx < lookup_size_; ++idx) {
    	while (lookup_[idx] != 0) {
    	  // Remove from the old lookup table.
    	  bucket_type* tmp = lookup_[idx];
    	  lookup_[idx] = tmp->hash_next;
    	  // Compute the new lookup index.
    	  size_type new_idx = tmp->hash % new_lookup_size;
    	  // Insert into the new table.
    	  tmp->hash_next = new_lookup[new_idx];
    	  new_lookup[new_idx] = tmp;
    	}
      }

      lookup_allocator::deallocate (lookup_, lookup_size_);
      lookup_ = new_lookup;
      lookup_size_ = new_lookup_size;
    }

    return true;
  }

//   pair<iterator, bool>
//   insert (const value_type& value)
//   {
//     // Compute the hash.
//     size_t hash = Hash::operator() (Selector::operator () (value));
//     // Compute the key into the lookup table.
//     size_type lookup_key = hash % lookup_size_;
//     // Begin a search.
//     for (bucket_type* bucket = lookup_[lookup_key];
// 	 bucket != 0;
// 	 bucket = bucket->next) {
//       if (bucket->hash == hash && Equal::operator() (Selector::operator () (bucket->value), Selector::operator () (value))) {
// 	// Found it.
// 	return make_pair (iterator (this, bucket), false);
//       }
//     }

//     bucket_type* bucket = bucket_allocator::allocate (1);
//     Allocator::construct (&bucket->value, value);
//     ++size_;
//     bucket->hash = hash;
//     bucket->next = lookup_[lookup_key];
//     lookup_[lookup_key] = bucket;

//     // Rehash when size_ / lookup_size_ > .80
//     // Rehash when size_ / lookup_size_ > 80 / 100
//     // Rehash when 100 * size_ > 80 * lookup_size_
//     if (100 * size_ > 80 * lookup_size_) {
//       size_type new_lookup_size = 2 * lookup_size_;
//       bucket_type** new_lookup = lookup_allocator::allocate (new_lookup_size);
//       for (size_type idx = 0; idx < new_lookup_size; ++idx) {
//     	new_lookup[idx] = 0;
//       }

//       for (size_type idx = 0; idx < lookup_size_; ++idx) {
//     	while (lookup_[idx] != 0) {
//     	  // Remove from the old lookup table.
//     	  bucket_type* tmp = lookup_[idx];
//     	  lookup_[idx] = tmp->next;
//     	  // Compute the new lookup index.
//     	  size_type new_idx = tmp->hash % new_lookup_size;
//     	  // Insert into the new table.
//     	  tmp->next = new_lookup[new_idx];
//     	  new_lookup[new_idx] = tmp;
//     	}
//       }

//       lookup_allocator::deallocate (lookup_, lookup_size_);
//       lookup_ = new_lookup;
//       lookup_size_ = new_lookup_size;
//     }

//     return make_pair (iterator (this, bucket), true);
//   }

//   void
//   erase (const_iterator pos)
//   {
//     // Find the bucket.
//     // Compute the key into the lookup table.
//     size_type lookup_key = pos.bucket_->hash % lookup_size_;
//     // Begin a search.
//     for (bucket_type** bucket = &lookup_[lookup_key];
// 	 *bucket != 0;
// 	 bucket = &((*bucket)->next)) {
//       if (*bucket == pos.bucket_) {
// 	// Found it.
// 	bucket_type* tmp = *bucket;
// 	*bucket = tmp->next;
// 	Allocator::destroy (&tmp->value);
// 	bucket_allocator::deallocate (tmp, 1);
// 	--size_;
//       }
//     }
//   }

  size_type
  erase (const key_type& key)
  {
    // Compute the hash.
    size_t hash = Hash::operator() (key);
    // Compute the key into the lookup table.
    size_type lookup_key = hash % lookup_size_;
    // Begin a search.
    for (bucket_type** bucket = &(lookup_[lookup_key]);
	 *bucket != 0;
	 bucket = &((*bucket)->hash_next)) {
      if ((*bucket)->hash == hash && Equal::operator() (Selector::operator () ((*bucket)->value), key)) {
	// Found it.
	// Remove it from the hash.
	bucket_type* tmp = *bucket;
	*bucket = tmp->hash_next;

	// Remove it from the linked list.
	bucket_type* prev = tmp->list_prev;
	bucket_type* next = tmp->list_next;

	if (prev != 0) {
	  prev->list_next = next;
	}
	else {
	  // We were the front.
	  front_ = next;
	}

	if (next != 0) {
	  next->list_prev = prev;
	}
	else {
	  // We were the back.
	  back_ = prev;
	}

	Allocator::destroy (&tmp->value);
	bucket_allocator::deallocate (tmp, 1);
	--size_;
	return 1;
      }
    }
    
    return 0;
  }

};

#endif /* __lin_assoc_impl_hpp__ */
