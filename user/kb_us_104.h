#ifndef KB_US_104_H
#define KB_US_104_H

#include <stddef.h>

#define KB_US_104_SCAN_CODE 1

typedef struct {
  size_t length;	/* The length of the ASCII string below. */
  ptrdiff_t string;	/* Relative offset to an array of chars. There is no null terminator. */
} kb_us_104_string_t;

#define KB_US_104_STRING 2

#endif /* KB_US_104_H */
