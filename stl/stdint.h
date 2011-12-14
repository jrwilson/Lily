#ifndef __stdint_h__
#define __stdint_h__

#ifdef __INT8_TYPE__
typedef __INT8_TYPE__ int8_t;
#else
typedef signed char int8_t;
#endif

#ifdef __INT_LEAST8_TYPE__
typedef __INT_LEAST8_TYPE__ int_least8_t;
#else
typedef signed char int_least8_t;
#endif

#ifdef __INT_FAST8_TYPE__
typedef __INT_FAST8_TYPE__ int_fast8_t;
#else
typedef signed char int_fast8_t;
#endif

#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__ uint8_t;
#else
typedef unsigned char uint8_t;
#endif

#ifdef __UINT_LEAST8_TYPE__
typedef __UINT_LEAST8_TYPE__ uint_least8_t;
#else
typedef unsigned char uint_least8_t;
#endif

#ifdef __UINT_FAST8_TYPE__
typedef __UINT_FAST8_TYPE__ uint_fast8_t;
#else
typedef unsigned char uint_fast8_t;
#endif

#ifdef __INT16_TYPE__
typedef __INT16_TYPE__ int16_t;
#else
typedef signed short int16_t;
#endif

#ifdef __INT_LEAST16_TYPE__
typedef __INT_LEAST16_TYPE__ int_least16_t;
#else
typedef signed short int_least16_t;
#endif

#ifdef __INT_FAST16_TYPE__
typedef __INT_FAST16_TYPE__ int_fast16_t;
#else
typedef signed short int_fast16_t;
#endif

#ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ uint16_t;
#else
typedef unsigned short uint16_t;
#endif

#ifdef __UINT_LEAST16_TYPE__
typedef __UINT_LEAST16_TYPE__ uint_least16_t;
#else
typedef unsigned short uint_least16_t;
#endif

#ifdef __UINT_FAST16_TYPE__
typedef __UINT_FAST16_TYPE__ uint_fast16_t;
#else
typedef unsigned short uint_fast16_t;
#endif

#ifdef __INT32_TYPE__
typedef __INT32_TYPE__ int32_t;
#else
typedef signed int int32_t;
#endif

#ifdef __INT_LEAST32_TYPE__
typedef __INT_LEAST32_TYPE__ int_least32_t;
#else
typedef signed int int_least32_t;
#endif

#ifdef __INT_FAST32_TYPE__
typedef __INT_FAST32_TYPE__ int_fast32_t;
#else
typedef signed int int_fast32_t;
#endif

#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ uint32_t;
#else
typedef unsigned int uint32_t;
#endif

#ifdef __UINT_LEAST32_TYPE__
typedef __UINT_LEAST32_TYPE__ uint_least32_t;
#else
typedef unsigned int uint_least32_t;
#endif

#ifdef __UINT_FAST32_TYPE__
typedef __UINT_FAST32_TYPE__ uint_fast32_t;
#else
typedef unsigned int uint_fast32_t;
#endif

#ifdef __INT64_TYPE__
typedef __INT64_TYPE__ int64_t;
#else
typedef signed long long int int64_t;
#endif

#ifdef __INT_LEAST64_TYPE__
typedef __INT_LEAST64_TYPE__ int_least64_t;
#else
typedef signed long long int int_least64_t;
#endif

#ifdef __INT_FAST64_TYPE__
typedef __INT_FAST64_TYPE__ int_fast64_t;
#else
typedef signed long long int int_fast64_t;
#endif

#ifdef __UINT64_TYPE__
typedef __UINT64_TYPE__ uint64_t;
#else
typedef unsigned long long int uint64_t;
#endif

#ifdef __UINT_LEAST64_TYPE__
typedef __UINT_LEAST64_TYPE__ uint_least64_t;
#else
typedef unsigned long long int uint_least64_t;
#endif

#ifdef __UINT_FAST64_TYPE__
typedef __UINT_FAST64_TYPE__ uint_fast64_t;
#else
typedef unsigned long long int uint_fast64_t;
#endif

#ifdef __INTMAX_TYPE__
typedef __INTMAX_TYPE__ intmax_t;
#endif

#ifdef __INTPTR_TYPE__
typedef __INTPTR_TYPE__ intintptr_t;
#else
typedef int intintptr_t;
#endif

#ifdef __UINTMAX_TYPE__
typedef __UINTMAX_TYPE__ uintmax_t;
#endif

#ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__ uintintptr_t;
#else
typedef unsigned int uintintptr_t;
#endif

#endif /* __stdint_h__ */
