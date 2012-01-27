#ifndef __identity_selector_hpp__
#define __identity_selector_hpp__

template <typename T>
struct identity_selector {
  const T&
  operator() (const T& t) const
  {
    return t;
  }
};

#endif /* __identity_selector_hpp__ */
