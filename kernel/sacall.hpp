#ifndef __sacall_hpp__
#define __sacall_hpp__

/*
  File
  ----
  sacall.hpp
  
  Description
  -----------
  System calls for the system automaton.

  Authors:
  Justin R. Wilson
*/

#include "aid.hpp"
#include "bid.hpp"

namespace sacall {
  
  enum {
    CREATE,
    BIND,
    LOOSE,
    DESTROY,
  };

  inline aid_t
  create (bid_t automaton_bid,
	  size_t automaton_size)
  {
    aid_t aid;
    asm ("int $0x82\n" : "=a"(aid) : "a"(CREATE), "b"(automaton_bid), "c"(automaton_size) :);
    return aid;
  }

  inline void
  bind (void)
  {
    asm ("int $0x82\n" : : "a"(BIND) :);
  }

  inline void
  loose (void)
  {
    asm ("int $0x82\n" : : "a"(LOOSE) :);
  }

  inline void
  destroy (void)
  {
    asm ("int $0x82\n" : : "a"(DESTROY) :);
  }

}

#endif /* __sacall_hpp__ */
