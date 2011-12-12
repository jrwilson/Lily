#ifndef __stddef_h__
#define __stddef_h__

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __SIZE_TYPE__ size_t;

#undef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif

#define offsetof(type,member) ((size_t)(&((type*)0)->member))

#endif /* __stddef_h__ */
