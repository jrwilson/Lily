#ifndef __types_hpp__
#define __types_hpp__

#include <stddef.h>

typedef char int8_t;
typedef unsigned char uint8_t;

#if __SIZEOF_SHORT__ == 2
typedef signed short int16_t;
typedef unsigned short uint16_t;
#endif 

#if __SIZEOF_INT__ == 4
typedef unsigned int uint32_t;
#endif

#if __SIZEOF_LONG_LONG__ == 8
typedef unsigned long long uint64_t;
#endif

#if __SIZEOF_INT__ == __SIZEOF_POINTER__
typedef unsigned int uintptr_t;
#endif

typedef int aid_t;
typedef int bid_t;

#define M_NO_COMPARE 0
#define M_EQUAL 1

#define M_INPUT 0
#define M_OUTPUT 1
#define M_INTERNAL 2

enum action_type_t {
  INPUT = M_INPUT,
  OUTPUT = M_OUTPUT,
  INTERNAL = M_INTERNAL,
};

enum compare_method_t {
  NO_COMPARE = M_NO_COMPARE,
  EQUAL = M_EQUAL,
};

#define M_NO_PARAMETER 0
#define M_PARAMETER 1
#define M_AUTO_PARAMETER 2

enum parameter_mode_t {
  NO_PARAMETER = M_NO_PARAMETER,
  PARAMETER = M_PARAMETER,
  AUTO_PARAMETER = M_AUTO_PARAMETER,
};

// Size of the temporary buffer used to store the values produced by output actions.
const size_t MAX_COPY_VALUE_SIZE = 512;

enum {
  CREATE = 0,
  BIND = 1,
  LOOSE = 2,
  DESTROY = 3,
  
  SBRK = 4,
  
  BINDING_COUNT = 5,
  
  BUFFER_CREATE = 6,
  BUFFER_COPY = 7,
  BUFFER_GROW = 8,
  BUFFER_APPEND = 9,
  BUFFER_ASSIGN = 10,
  BUFFER_MAP = 11,
  BUFFER_UNMAP = 12,
  BUFFER_DESTROY = 13,
  BUFFER_SIZE = 14,
};

#endif /* __types_hpp__ */
