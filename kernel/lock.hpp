#ifndef __lock_hpp__
#define __lock_hpp__

#include "mutex.hpp"

class lock {
private:
  mutex& m_;

public:
  lock (mutex& m) :
    m_ (m)
  {
    m_.lock ();
  }

  ~lock ()
  {
    m_.unlock ();
  }

};

#endif /* __lock_hpp__ */
