#ifndef __utility_hpp__
#define __utility_hpp__

template <typename First,
	  typename Second>
struct pair {
  typedef First first_type;
  typedef Second second_type;
  First first;
  Second second;

  pair () { }

  pair (const First& t,
	const Second& u) :
    first (t),
    second (u)
  { }

  template <typename V,
	    typename W>
  pair (const pair<V, W>& other) :
    first (other.first),
    second (other.second)
  { }
};

template <typename First,
	  typename Second>
pair<First, Second>
make_pair (First first,
	   Second second)
{
  return pair<First, Second> (first, second);
}

#endif /* __utility_hpp__ */
