#ifndef ARGV_H
#define ARGV_H

#include <buffer_file.h>

typedef struct {
  buffer_file_t argv_bf;
  buffer_file_t string_bf;
  size_t argc;
} argv_t;

int
argv_initw (argv_t* a,
	    bd_t* bda,
	    bd_t* bdb);

int
argv_append (argv_t* a,
	     const char* str);

int
argv_initr (argv_t* a,
	    bd_t bda,
	    bd_t bdb,
	    size_t* argc);

const char*
argv_arg (argv_t* a,
	  size_t idx);

#endif /* ARGV_H */
