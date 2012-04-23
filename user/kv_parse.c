#include "kv_parse.h"
#include <ctype.h>
#include <dymem.h>
#include <string.h>

int
kv_parse (char** key,
	  char** value,
	  const char** ptr,
	  const char* end)
{
  *key = 0;
  *value = 0;
  const char* p = *ptr;

  /* Skip whitespace and '='. */
  while (p != end && (isspace (*p) || *p == '=')) {
    ++p;
  }

  if (p == end) {
    *ptr = p;
    return 0;
  }

  const char* key_begin = p;

  /* Parse the key. */
  while (p != end && *p != '=' && !isspace (*p)) {
    ++p;
  }

  const char* key_end = p;

  if (key_end == key_begin) {
    /* Done. */
    *ptr = p;
    return 0;
  }

  *key = malloc (key_end - key_begin + 1);
  //assert (*key != NULL);
  memcpy (*key, key_begin, key_end - key_begin);
  (*key)[key_end - key_begin] = '\0';

  /* Skip whitespace. */
  while (p != end && isspace (*p)) {
    ++p;
  }

  if (p == end) {
    *ptr = p;
    return 0;
  }

  /* Find '='. */
  if (*p == '=') {
    ++p;

    /* Skip whitespace and '='. */
    while (p != end && (isspace (*p) || *p == '=')) {
      ++p;
    }
    
    if (p == end) {
      *ptr = p;
      return 0;
    }

    const char* value_begin = p;
    
    /* Parse the value. */
    while (p != end && *p != '=' && !isspace (*p)) {
      ++p;
    }
    
    const char* value_end = p;
    
    if (value_end == value_begin) {
      /* Done. */
      *ptr = p;
      return 0;
    }
    
    *value = malloc (value_end - value_begin + 1);
    //assert (*value != NULL);
    memcpy (*value, value_begin, value_end - value_begin);
    (*value)[value_end - value_begin] = '\0';
  }

  *ptr = p;
  return 0;
}
