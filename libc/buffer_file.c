#include "buffer_file.h"
#include "automaton.h"
#include "string.h"
#include <stdarg.h>

int
buffer_file_initw (buffer_file_t* bf,
		   lily_error_t* err,
		   bd_t bd)
{
  bf->bd = bd;
  bf->bd_size = buffer_size (err, bd);
  if (bf->bd_size == -1) {
    return -1;
  }
  bf->capacity = bf->bd_size * pagesize ();
  if (bf->bd_size != 0) {
    bf->ptr = buffer_map (err, bd);
    if (bf->ptr == 0) {
      return -1;
    }
  }
  else {
    bf->ptr = 0;
  }
  bf->size = sizeof (size_t);
  bf->position = sizeof (size_t);
  bf->can_update = true;

  return 0;
}

int
buffer_file_write (buffer_file_t* bf,
		   lily_error_t* err,
		   const void* ptr,
		   size_t size)
{
  if (!bf->can_update) {
    return -1;
  }

  size_t new_position = bf->position + size;
  if (new_position < bf->position) {
    /* Overflow. */
    return -1;
  }

  /* Resize if necessary. */
  if (bf->capacity < new_position) {
    buffer_unmap (err, bf->bd);
    bf->capacity = ALIGN_UP (new_position, pagesize ());
    bf->bd_size = bf->capacity / pagesize ();
    if (buffer_resize (err, bf->bd, bf->bd_size) != 0) {
      return -1;
    }
    bf->ptr = buffer_map (err, bf->bd);
    if (bf->ptr == 0) {
      return -1;
    }
  }

  memcpy (bf->ptr + bf->position, ptr, size);
  bf->position = new_position;
  if (bf->position > bf->size) {
    bf->size = bf->position;
    *((size_t*)bf->ptr) = bf->size;
  }

  return 0;
}

int
buffer_file_put (buffer_file_t* bf,
		 lily_error_t* err,
		 char c)
{
  if (!bf->can_update) {
    return -1;
  }
  
  size_t new_position = bf->position + 1;
  if (new_position < bf->position) {
    /* Overflow. */
    return -1;
  }
  
  /* Resize if necessary. */
  if (bf->capacity < new_position) {
    buffer_unmap (err, bf->bd);
    bf->capacity = ALIGN_UP (new_position, pagesize ());
    bf->bd_size = bf->capacity / pagesize ();
    if (buffer_resize (err, bf->bd, bf->bd_size) != 0) {
      return -1;
    }
    bf->ptr = buffer_map (err, bf->bd);
    if (bf->ptr == 0) {
      return -1;
    }
  }
  
  *((char*)(bf->ptr + bf->position)) = c;
  bf->position = new_position;
  if (bf->position > bf->size) {
    bf->size = bf->position;
    *((size_t*)bf->ptr) = bf->size;
  }

  return 0;
}

int
buffer_file_puts (buffer_file_t* bf,
		  lily_error_t* err,
		  const char* s)
{
  while (*s != '\0') {
    if (buffer_file_put (bf, err, *s) != 0) {
      return -1;
    }
    ++s;
  }

  return 0;
}

void
buffer_file_truncate (buffer_file_t* bf)
{
  bf->position = sizeof (size_t);
  bf->size = sizeof (size_t);
}

int
buffer_file_initr (buffer_file_t* bf,
		   lily_error_t* err,
		   bd_t bd)
{
  bf->bd = bd;
  bf->bd_size = buffer_size (err, bd);
  if (bf->bd_size == -1) {
    return -1;
  }
  bf->capacity = bf->bd_size * pagesize ();
  bf->ptr = buffer_map (err, bd);
  if (bf->ptr == 0) {
    return -1;
  }
  bf->size = *((size_t*)bf->ptr);
  bf->position = sizeof (size_t);
  bf->can_update = false;

  return 0;
}

const void*
buffer_file_readp (buffer_file_t* bf,
		   size_t size)
{
  if (bf->can_update) {
    /* Error: Cannot form pointers. */
    return 0;
  }

  if (bf->position > bf->size) {
    return 0;
  }

  if (size > (bf->size - bf->position)) {
    /* Error: Not enough data. */
    return 0;
  }
  
  /* Success.
     Return the current position. */
  void* retval = bf->ptr + bf->position;
  /* Advance. */
  bf->position += size;
  return retval;
}

int
buffer_file_read (buffer_file_t* bf,
		  void* ptr,
		  size_t size)
{
  if (bf->position > bf->size) {
    return -1;
  }

  if (size > (bf->size - bf->position)) {
    /* Error: Not enough data. */
    return -1;
  }

  memcpy (ptr, bf->ptr + bf->position, size);
  bf->position += size;

  return 0;
}

bd_t
buffer_file_bd (const buffer_file_t* bf)
{
  return bf->bd;
}

size_t
buffer_file_size (const buffer_file_t* bf)
{
  return bf->size - sizeof (size_t);
}

size_t
buffer_file_position (const buffer_file_t* bf)
{
  return bf->position - sizeof (size_t);
}

int
buffer_file_seek (buffer_file_t* bf,
		  size_t position)
{
  position += sizeof (size_t);

  if (position < sizeof (size_t)) {
    return -1;
  }

  bf->position = position;
  return 0;
}

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
  char left[1];
  size_t left_start;
  size_t left_size;
  char right[10];
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
  buf->left_start = 0;
  buf->left_size = 0;
  buf->right_size = 0;
  buf->alternate_form = false;
  buf->zero_padded = false;
  buf->left_adjust = false;
  buf->space = false;
  buf->sign = false;
  buf->width = 0;
  buf->precision_specified = false;
  buf->precision = 0;
  buf->length = NORMAL;
}

static bool
fbuf_empty_left (const fbuf_t* buf)
{
  return buf->left_start == buf->left_size;
}

static void
fbuf_push_left (fbuf_t* buf,
		char c)
{
  buf->left[buf->left_size++] = c;
}

static char
fbuf_pop_left (fbuf_t* buf)
{
  return buf->left[buf->left_start++];
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

int
bfprintf (buffer_file_t* bf,
	  lily_error_t* err,
	  const char* format,
	  ...)
{
  va_list ap;
  fbuf_t fbuf;

  fbuf_reset (&fbuf);
  va_start (ap, format);

  while (*format != '\0') {

    const char c = *format;
    
    switch (fbuf.state) {
    case COPY:
      if (c != '%') {
  	if (buffer_file_put (bf, err, c) != 0) {
  	  va_end (ap);
  	  return -1;
  	}
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
  	fbuf.width = fbuf.width * 10 + (c - '0');
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
  	fbuf.precision_specified = true;
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
    	    if (buffer_file_put (bf, err, '0') != 0) {
  	      va_end (ap);
    	      return -1;
    	    }
    	  }
    	  else if (num != -2147483648) {
    	    if (num < 0) {
    	      fbuf_push_left (&fbuf, '-');
    	      num *= -1;
    	    }
	    
    	    while (num != 0) {
    	      fbuf_push_right (&fbuf, '0' + num % 10);
    	      num /= 10;
    	    }
	    
    	    while (!fbuf_empty_left (&fbuf)) {
    	      if (buffer_file_put (bf, err, fbuf_pop_left (&fbuf)) != 0) {
  		va_end (ap);
    		return -1;
    	      }
    	    }
	    
    	    while (!fbuf_empty_right (&fbuf)) {
    	      if (buffer_file_put (bf, err, fbuf_pop_right (&fbuf)) != 0) {
  		va_end (ap);
    		return -1;
    	      }
    	    }
    	  }
    	  else {
    	    if (buffer_file_puts (bf, err, "-2147483648") != 0) {
  	      va_end (ap);
    	      return -1;
    	    }
    	  }
  	}
	fbuf_reset (&fbuf);
        ++format;
        break;
      /* case 'o': */
      /* case 'u': */
      case 'x':
      /* case 'X': */
  	/* TODO */
    	{
    	  unsigned int num = va_arg (ap, unsigned int);
	  if (fbuf.alternate_form) {
    	    if (buffer_file_puts (bf, err, "0x") != 0) {
  	      va_end (ap);
    	      return -1;
    	    }
	  }
    	  if (num == 0) {
    	    if (buffer_file_put (bf, err, '0') != 0) {
  	      va_end (ap);
    	      return -1;
    	    }
    	  }
    	  else {
    	    while (num != 0) {
    	      fbuf_push_right (&fbuf, to_hex (num % 16));
    	      num /= 16;
    	    }

    	    while (!fbuf_empty_left (&fbuf)) {
    	      if (buffer_file_put (bf, err, fbuf_pop_left (&fbuf)) != 0) {
  		va_end (ap);
    		return -1;
    	      }
    	    }

    	    while (!fbuf_empty_right (&fbuf)) {
    	      if (buffer_file_put (bf, err, fbuf_pop_right (&fbuf)) != 0) {
  		va_end (ap);
    		return -1;
    	      }
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
  	  if (buffer_file_put (bf, err, x) != 0) {
  	    va_end (ap);
  	    return -1;
  	  }
  	}
	fbuf_reset (&fbuf);
	++format;
      	break;
      case 's':
    	{
    	  const char* str = va_arg (ap, const char*);
  	  if (fbuf.precision_specified) {
  	    for (size_t count = 0; count != fbuf.precision && *str != '\0'; ++count, ++str) {
  	      if (buffer_file_put (bf, err, *str) != 0) {
  		va_end (ap);
  		return -1;
  	      }
  	    }
  	  }
  	  else {
  	    if (buffer_file_puts (bf, err, str) != 0) {
  	      va_end (ap);
  	      return -1;
  	    }
  	  }
    	}
	fbuf_reset (&fbuf);
	++format;
      	break;
      /* case 'p': */
      /* 	/\* TODO *\/ */
      /* 	break; */
      /* case 'n': */
      /* 	break; */
      case '%':
  	if (buffer_file_put (bf, err, '%') != 0) {
  	  va_end (ap);
  	  return -1;
  	}
	fbuf_reset (&fbuf);
	++format;
      	break;
      default:
  	/* Error. */
	va_end (ap);
	return -1;
  	break;
      }
      break;
    }
  }
  
  va_end (ap);
  return 0;
}
