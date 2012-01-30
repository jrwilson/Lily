#ifndef __functional_hpp__
#define __functional_hpp__

template <typename A1,
	  typename Result>
struct unary_function {
  typedef A1 first_argument_type;
  typedef Result result_type;
};

template <typename A1,
	  typename A2,
	  typename Result>
struct binary_function {
  typedef A1 first_argument_type;
  typedef A2 second_argument_type;
  typedef Result result_type;
};

template <typename T>
struct equal_to : public binary_function<T, T, bool> {
  bool
  operator() (const T& x,
	      const T& y) const
  {
    return x == y;
  }
};

template <typename T>
struct less : public binary_function<T, T, bool> {
  bool
  operator() (const T& x,
	      const T& y) const
  {
    return x < y;
  }
};

template <typename T>
struct hash : public unary_function<T, size_t> {
  size_t
  operator() (T value) const;
};

template <typename T>
struct hash<T*> : public unary_function<T*, size_t> {
  size_t
  operator() (T* ptr) const
  {
    return reinterpret_cast<size_t> (ptr);
  }
};

template <>
struct hash<int> : public unary_function<int, size_t> {
  size_t
  operator() (int value) const
  {
    return value;
  }
};

template <>
struct hash<unsigned short> : public unary_function<unsigned short, size_t> {
  size_t
  operator() (unsigned short value) const
  {
    return value;
  }
};

template <>
struct hash<unsigned int> : public unary_function<unsigned int, size_t> {
  size_t
  operator() (unsigned int value) const
  {
    return value;
  }
};

template <>
struct hash<unsigned long> : public unary_function<unsigned long, size_t> {
  size_t
  operator() (unsigned long value) const
  {
    return value;
  }
};

#endif /* __functional_hpp__ */
