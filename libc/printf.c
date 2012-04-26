#include "printf.h"

#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

typedef enum {
  COPY,
  FLAGS,
  WIDTH1,
  WIDTH,
  PRECISION1,
  PRECISION,
  LENGTH,
  LENGTH2,
  CONVERSION,
} fbuf_state_t;

typedef enum {
  NORMAL,
  CHAR,
  SHORT,
  LONG,
  LONGLONG,
  LONGDOUBLE,
  MAX,
  SIZE,
  PTRDIFF
} length_t;

typedef struct {
  fbuf_state_t state;
  char right[11];
  size_t right_size;
  bool alternate_form;
  bool zero_padded;
  bool left_adjust;
  bool space;
  bool sign;
  size_t width;
  bool precision_specified;
  size_t precision;
  length_t length;
} fbuf_t;

static void
fbuf_reset (fbuf_t* buf)
{
  buf->state = COPY;
}

static void
fbuf_start_conversion (fbuf_t* buf)
{
  buf->state = FLAGS;
  buf->right_size = 0;
  buf->alternate_form = false;
  buf->zero_padded = false;
  buf->left_adjust = false;
  buf->space = false;
  buf->sign = false;
  buf->width = 0;
  buf->precision_specified = false;
  buf->precision = 1;
  buf->length = NORMAL;
}

static bool
fbuf_empty_right (const fbuf_t* buf)
{
  return buf->right_size == 0;
}

static void
fbuf_push_right (fbuf_t* buf,
		char c)
{
  buf->right[buf->right_size++] = c;
}

static char
fbuf_pop_right (fbuf_t* buf)
{
  return buf->right[--buf->right_size];
}

static char
to_hex (int x)
{
  if (x < 10) {
    return x + '0';
  }
  else {
    return x - 10 + 'a';
  }
}

static void
put (printf_t* ptr,
     unsigned char c)
{
  ptr->put (ptr->aux, c);
}

static void
puts (printf_t* ptr,
      const char* str)
{
  while (*str != '\0') {
    put (ptr, *str);
    ++str;
  }
}
 
int
printf (printf_t* ptr,
	const char* format,
	va_list ap)
{
  fbuf_t fbuf;
  fbuf_reset (&fbuf);

  while (*format != '\0') {

    const char c = *format;
    
    switch (fbuf.state) {
    case COPY:
      if (c != '%') {
  	put (ptr, c);
      }
      else {
  	fbuf_start_conversion (&fbuf);
      }
      ++format;
      break;

    case FLAGS:
      switch (c) {
      case '#':
  	fbuf.alternate_form = true;
	++format;
  	break;
      case '0':
  	fbuf.zero_padded = true;
	++format;
  	break;
      case '-':
  	fbuf.left_adjust = true;
	++format;
  	break;
      case ' ':
  	fbuf.space = true;
	++format;
  	break;
      case '+':
  	fbuf.sign = true;
	++format;
  	break;
      default:
  	fbuf.state = WIDTH1;
  	break;
      }
      break;

    case WIDTH1:
      switch (c) {
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
  	fbuf.width = (c - '0');
	fbuf.state = WIDTH;
        ++format;
  	break;
      default:
  	fbuf.state = PRECISION1;
  	break;
      }
      break;

    case WIDTH:
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
  	fbuf.width = fbuf.width * 10 + (c - '0');
        ++format;
  	break;
      default:
  	fbuf.state = PRECISION1;
  	break;
      }
      break;

    case PRECISION1:
      if (c == '.') {
  	fbuf.state = PRECISION;
  	fbuf.precision = 0;
        ++format;
      }
      else {
  	fbuf.state = LENGTH;
      }
      break;

    case PRECISION:
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
  	fbuf.precision = fbuf.precision * 10 + (c - '0');
        ++format;
  	break;
      default:
  	fbuf.state = LENGTH;
  	break;
      }
      break;

    case LENGTH:
      switch (c) {
      case 'h':
  	fbuf.length = SHORT;
  	fbuf.state = LENGTH2;
	++format;
  	break;
      case 'l':
  	fbuf.length = LONG;
  	fbuf.state = LENGTH2;
	++format;
  	break;
      case 'L':
  	fbuf.length = LONGDOUBLE;
        fbuf.state = CONVERSION;
	++format;
        break;
      case 'j':
  	fbuf.length = MAX;
        fbuf.state = CONVERSION;
	++format;
        break;
      case 'z':
  	fbuf.length = SIZE;
        fbuf.state = CONVERSION;
	++format;
        break;
      case 't':
  	fbuf.length = PTRDIFF;
        fbuf.state = CONVERSION;
	++format;
        break;
      default:
  	fbuf.state = CONVERSION;
  	break;
      }
      break;

    case LENGTH2:
      if (fbuf.length == SHORT && c == 'h') {
  	fbuf.length = CHAR;
	++format;
      }
      else if (fbuf.length == LONG && c == 'l') {
  	fbuf.length = LONGLONG;
	++format;
      }
      fbuf.state = CONVERSION;
      break;

    case CONVERSION:
      switch (c) {
      case 'd':
      case 'i':
  	/* TODO */
    	{
    	  int num = va_arg (ap, int);
    	  if (num == 0) {
    	    put (ptr, '0');
    	  }
    	  else if (num != INT_MIN) {
	    bool negative = false;
    	    if (num < 0) {
    	      num *= -1;
	      negative = true;
    	    }
	    
    	    while (num != 0) {
    	      fbuf_push_right (&fbuf, '0' + num % 10);
    	      num /= 10;
    	    }
	    
	    if (negative) {
	      fbuf_push_right (&fbuf, '-');
	    }
	    
    	    while (!fbuf_empty_right (&fbuf)) {
    	      put (ptr, fbuf_pop_right (&fbuf));
    	    }
    	  }
    	  else {
    	    puts (ptr, "-2147483648");
    	  }
  	}
	fbuf_reset (&fbuf);
        ++format;
        break;
      /* case 'o': */
      case 'u':
  	/* TODO */
    	{
    	  unsigned int num = va_arg (ap, unsigned int);
    	  if (num == 0) {
    	    put (ptr, '0');
    	  }
    	  else {
    	    while (num != 0) {
    	      fbuf_push_right (&fbuf, '0' + num % 10);
    	      num /= 10;
    	    }
	    
	    if (fbuf.width != 0) {
	      for (size_t count = fbuf.right_size; count < fbuf.width; ++count) {
		put (ptr, fbuf.zero_padded ? '0' : ' ');
	      }
	    }
	    
    	    while (!fbuf_empty_right (&fbuf)) {
    	      put (ptr, fbuf_pop_right (&fbuf));
    	    }
    	  }
  	}
	fbuf_reset (&fbuf);
        ++format;
        break;
      case 'x':
      /* case 'X': */
  	/* TODO */
    	{
    	  unsigned int num = va_arg (ap, unsigned int);
	  if (fbuf.alternate_form) {
    	    puts (ptr, "0x");
	  }
    	  if (num == 0) {
    	    put (ptr, '0');
    	  }
    	  else {
    	    while (num != 0) {
    	      fbuf_push_right (&fbuf, to_hex (num % 16));
    	      num /= 16;
    	    }

    	    while (!fbuf_empty_right (&fbuf)) {
    	      put (ptr, fbuf_pop_right (&fbuf));
    	    }
    	  }
    	}
	fbuf_reset (&fbuf);
        ++format;
      	break;
      /* case 'e': */
      /* case 'E': */
      /* 	break; */
      /* case 'f': */
      /* case 'F': */
      /* 	break; */
      /* case 'g': */
      /* case 'G': */
      /* 	break; */
      /* case 'a': */
      /* case 'A': */
      /* 	break; */
      case 'c':
  	{
  	  unsigned char x = va_arg (ap, int);
  	  put (ptr, x);
  	}
	fbuf_reset (&fbuf);
	++format;
      	break;
      case 's':
    	{
    	  const char* str = va_arg (ap, const char*);
  	  if (fbuf.precision_specified) {
  	    for (size_t count = 0; count != fbuf.precision && *str != '\0'; ++count, ++str) {
  	      put (ptr, *str);
  	    }
  	  }
  	  else {
  	    puts (ptr, str);
  	  }
    	}
	fbuf_reset (&fbuf);
	++format;
      	break;
      case 'p':
    	{
    	  unsigned int num = va_arg (ap, unsigned int);
	  puts (ptr, "0x");
    	  if (num == 0) {
    	    put (ptr, '0');
    	  }
    	  else {
    	    while (num != 0) {
    	      fbuf_push_right (&fbuf, to_hex (num % 16));
    	      num /= 16;
    	    }
	    
    	    while (!fbuf_empty_right (&fbuf)) {
    	      put (ptr, fbuf_pop_right (&fbuf));
    	    }
    	  }
    	}
	fbuf_reset (&fbuf);
        ++format;
      	break;
      /* case 'n': */
      /* 	break; */
      case '%':
  	put (ptr, '%');
	fbuf_reset (&fbuf);
	++format;
      	break;
      default:
  	/* Error. */
	return -1;
  	break;
      }
      break;
    }
  }

  return 0;
}
