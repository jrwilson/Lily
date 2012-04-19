#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>

typedef struct printf_struct printf_t;

struct printf_struct {
  void* aux;
  void (*put) (void*, unsigned char c);
};

int
printf (printf_t* ptr,
	const char* format,
	va_list ap);

#endif /* PRINTF_H */
