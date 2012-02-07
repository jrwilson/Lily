#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stddef.h>

typedef struct {
  size_t count;		/* The number of scan codes in the array of scan codes. */
  ptrdiff_t codes;	/* Relative offset to an array of unsigned chars representing the scan codes. */
} keyboard_scan_code_t;

#define KEYBOARD_SCAN_CODE 2

#endif /* KEYBOARD_H */
