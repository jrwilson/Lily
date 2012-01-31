#ifndef __uno_assoc_impl_hpp__
#define __uno_assoc_impl_hpp__

/* Unordered Associative Container Implementation for unordered_set and unordered_map. */

#include "functional.hpp"
#include "utility.hpp"

template <typename Value>
struct uno_assoc_bucket {
  Value value;
  size_t hash;
  uno_assoc_bucket* next;
};

template <typename Key,
	  typename Value,
	  typename Selector,
	  typename Hash,
	  typename Equal,
	  typename Allocator>
class uno_assoc_cont : public Allocator::template rebind<uno_assoc_bucket<Value> >::other,
		       public Allocator::template rebind<uno_assoc_bucket<Value*> >::other,
		       public Selector,
		       public Hash,
		       public Equal {
private:
  typedef uno_assoc_cont<Key, Value, Selector, Hash, Equal, Allocator> cont_type;
  typedef uno_assoc_bucket<Value> bucket_type;
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
      size_type lookup_idx = (bucket_->hash % map_->lookup_size_) + 1;
      bucket_ = bucket_->next;

      for (; bucket_ == 0 && lookup_idx != map_->lookup_size_; ++lookup_idx) {
	bucket_ = map_->lookup_[lookup_idx];
      }

      return *this;
    }
  };

  typedef Hash hasher;
  typedef Equal key_equal;

private:

  bucket_type** lookup_;
  size_type lookup_size_;
  size_type size_;


public:
  uno_assoc_cont ()
  {
    lookup_ = lookup_allocator::allocate (2);
    lookup_size_ = 2;
    size_ = 0;
    for (size_type idx = 0; idx != lookup_size_; ++idx) {
      lookup_[idx] = 0;
    }
  }

  uno_assoc_cont (const uno_assoc_cont& other) :
    bucket_allocator (other),
    Allocator::template rebind<uno_assoc_bucket<Value*> >::other (other)
  {
    lookup_ = lookup_allocator::allocate (other.lookup_size_);
    lookup_size_ = other.lookup_size_;
    size_ = other.size_;
    for (size_type idx = 0; idx != lookup_size_; ++idx) {
      lookup_[idx] = 0;

      for (bucket_type* ptr = other.lookup_[idx]; ptr != 0; ptr = ptr->next) {
	bucket_type* bucket = bucket_allocator::allocate (1);
	Allocator::construct (&bucket->value, ptr->value);
	bucket->hash = ptr->hash;
	bucket->next = lookup_[idx];
	lookup_[idx] = bucket;
      }
    }
  }

  void
  clear ()
  {
    for (size_type idx = 0; idx < lookup_size_; ++idx) {
      while (lookup_[idx] != 0) {
	bucket_type* tmp = lookup_[idx];
	lookup_[idx] = tmp->next;
	Allocator::destroy (&tmp->value);
	bucket_allocator::deallocate (tmp, 1);
      }
    }
    size_ = 0;
  }

  ~uno_assoc_cont ()
  {
    clear ();
    lookup_allocator::deallocate (lookup_, lookup_size_);
  }

  size_type
  size () const
  {
    return size_;
  }

  const_iterator
  begin () const
  {
    for (size_type idx = 0; idx < lookup_size_; ++idx) {
      if (lookup_[idx] != 0) {
	return iterator (this, lookup_[idx]);
      }
    }

    return end ();
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
	 bucket = bucket->next) {
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

  pair<iterator, bool>
  insert (const value_type& value)
  {
    // Compute the hash.
    size_t hash = Hash::operator() (Selector::operator () (value));
    // Compute the key into the lookup table.
    size_type lookup_key = hash % lookup_size_;
    // Begin a search.
    for (bucket_type* bucket = lookup_[lookup_key];
	 bucket != 0;
	 bucket = bucket->next) {
      if (bucket->hash == hash && Equal::operator() (Selector::operator () (bucket->value), Selector::operator () (value))) {
	// Found it.
	return make_pair (iterator (this, bucket), false);
      }
    }

    bucket_type* bucket = bucket_allocator::allocate (1);
    Allocator::construct (&bucket->value, value);
    ++size_;
    bucket->hash = hash;
    bucket->next = lookup_[lookup_key];
    lookup_[lookup_key] = bucket;

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
    	  lookup_[idx] = tmp->next;
    	  // Compute the new lookup index.
    	  size_type new_idx = tmp->hash % new_lookup_size;
    	  // Insert into the new table.
    	  tmp->next = new_lookup[new_idx];
    	  new_lookup[new_idx] = tmp;
    	}
      }

      lookup_allocator::deallocate (lookup_, lookup_size_);
      lookup_ = new_lookup;
      lookup_size_ = new_lookup_size;
    }

    return make_pair (iterator (this, bucket), true);
  }

  void
  erase (const_iterator pos)
  {
    // Find the bucket.
    // Compute the key into the lookup table.
    size_type lookup_key = pos.bucket_->hash % lookup_size_;
    // Begin a search.
    for (bucket_type** bucket = &lookup_[lookup_key];
	 *bucket != 0;
	 bucket = &((*bucket)->next)) {
      if (*bucket == pos.bucket_) {
	// Found it.
	bucket_type* tmp = *bucket;
	*bucket = tmp->next;
	Allocator::destroy (&tmp->value);
	bucket_allocator::deallocate (tmp, 1);
	--size_;
	break;
      }
    }
  }

  size_type
  erase (const key_type& key)
  {
    // Compute the hash.
    size_t hash = Hash::operator() (key);
    // Compute the key into the lookup table.
    size_type lookup_key = hash % lookup_size_;
    // Begin a search.
    for (bucket_type** bucket = &lookup_[lookup_key];
	 *bucket != 0;
	 bucket = &((*bucket)->next)) {
      if ((*bucket)->hash == hash && Equal::operator() (Selector::operator () ((*bucket)->value), key)) {
	// Found it.
	bucket_type* tmp = *bucket;
	*bucket = tmp->next;
	Allocator::destroy (&tmp->value);
	bucket_allocator::deallocate (tmp, 1);
	--size_;
	return 1;
      }
    }

    return 0;
  }

};

#endif /* __uno_assoc_impl_hpp__ */
