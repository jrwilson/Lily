#ifndef __bitset_hpp__
#define __bitset_hpp__

template <size_t N>
class bitset {
private:
  uint32_t data_[N / 32];

  class reference {
  private:
    bitset& bs_;
    size_t idx_;

    friend class bitset;

    reference (bitset& bs,
	       size_t idx) :
      bs_ (bs),
      idx_ (idx)
    { }

  public:
    ~reference () { }

    operator bool () const
    {
      // To avoid recursion.
      return const_cast<const bitset&> (bs_)[idx_];
    }

    reference&
    operator= (bool v)
    {
      bs_.set (idx_, v);
      return *this;
    }

    reference&
    operator= (const reference& r)
    {
      bs_.set (idx_, r);
      return *this;
    }

    reference&
    flip ()
    {
      bs_.flip (idx_);
      return *this;
    }

    bool
    operator~ () const
    {
      // To avoid recursion.
      return !const_cast<const bitset&> (bs_)[idx_];
    }
  };

public:
  bool
  operator[] (size_t idx) const
  {
    return (data_[idx >> 5] & (1 << (idx & ((1 << 5) - 1)))) != 0;
  }

  reference
  operator[] (size_t idx)
  {
    return reference (*this, idx);
  }

  bitset&
  set (size_t idx,
       bool val = true)
  {
    if (val) {
      data_[idx >> 5] |= (1 << (idx & ((1 << 5) - 1)));
    }
    else {
      data_[idx >> 5] &= ~(1 << (idx & ((1 << 5) - 1)));
    }
    return *this;
  }
};

#endif /* __bitset_hpp__ */
