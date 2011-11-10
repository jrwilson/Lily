#ifndef __types_h__
#define __types_h__

/*
  File
  ----
  types.h
  
  Description
  -----------
  Declare types.

  Authors:
  Justin R. Wilson
*/

/* #define ARCH8 */
/* #define ARCH16 */
#define ARCH32
/* #define ARCH64 */

typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef uint32_t size_t;

typedef size_t parameter_t;
typedef size_t value_t;

#endif /* __types_h__ */



