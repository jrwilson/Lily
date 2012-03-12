#ifndef __mutex_hpp__
#define __mutex_hpp__

class mutex {
private:
  bool state_;
public:
  mutex () :
    state_ (false)
  { }

  void
  lock () {
    // TODO
    kassert (!state_);
    state_ = true;
  }

  void
  unlock () {
    // TODO
    kassert (state_);
    state_ = false;
  }
};

#endif /* __mutex_hpp__ */
