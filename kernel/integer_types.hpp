#ifndef __types_hpp__
#define __types_hpp__

#include <stddef.h>
#include "lily/limits.h"

// TODO:  Currently, <stdint.h> is not available.
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

#endif /* __types_hpp__ */
