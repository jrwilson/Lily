#include "kout.hpp"

// Colors.
static const unsigned char BLACK = 0;
static const unsigned char BLUE = 1;
static const unsigned char GREEN = 2;
static const unsigned char CYAN = 3;
static const unsigned char RED = 4;
static const unsigned char MAGENTA = 5;
static const unsigned char BROWN = 6;
static const unsigned char LIGHT_GREY = 7;
static const unsigned char DARK_GREY = 8;
static const unsigned char LIGHT_BLUE = 9;
static const unsigned char LIGHT_GREEN = 10;
static const unsigned char LIGHT_CYAN = 11;
static const unsigned char LIGHT_RED = 12;
static const unsigned char LIGHT_MAGENTA = 13;
static const unsigned char LIGHT_BROWN = 14;
static const unsigned char WHITE = 15;

// void
// kputs (const char* string)
// {
//   /* Pointer to the text framebuffer. */
//   unsigned short* videoram = (unsigned short*)VIDEORAM;

//   for (; *string != 0; ++string) {
//     /* Scroll if we are at the bottom of the screen. */
//     if (y_location == HEIGHT) {
//       unsigned int x, y;
//       for (y = 0; y < HEIGHT - 1; ++y) {
// 	for (x = 0; x < WIDTH; ++x) {
// 	  videoram[y * WIDTH + x] = videoram[(y + 1) * WIDTH + x];
// 	}
//       }
//       /* Fill last line with spaces. */
//       for (x = 0; x < WIDTH; ++x) {
// 	videoram[y * WIDTH + x] = (BLACK << 12) | (WHITE << 8) | ' ';
//       }
//       --y_location;
//     }

//     switch (*string) {
//     case '\b':
//       if (x_location > 0) {
// 	--x_location;
//       }
//       break;
//     case '\t':
//       /* A tab is a position divisible by 8. */
//       x_location = (x_location + 8) & ~(8-1);
//       break;
//     case '\n':
//       x_location = 0;
//       ++y_location;
//       break;
//     case '\r':
//       x_location = 0;
//       break;
//     default:
//       /* Print the character using black on white. */
//       videoram[y_location * WIDTH + x_location] = (BLACK << 12) | (WHITE << 8) | *string;
//       /* Advance the cursor. */
//       ++x_location;
//       if (x_location == WIDTH) {
// 	++y_location;
// 	x_location = 0;
//       }
//     }
//   }
// }

// static char
// to_hex (unsigned int n)
// {
//   if (n < 10) {
//     return n + '0';
//   }
//   else {
//     return n - 10 + 'A';
//   }
// }

// void
// kputx8 (unsigned char n)
// {
//   char buf[5];
//   buf[4] = 0;
//   buf[3] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[2] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[1] = 'x';
//   buf[0] = '0';
//   kputs (buf);
// }

// void
// kputx32 (uint32_t n)
// {
//   char buf[11];
//   buf[10] = 0;
//   buf[9] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[8] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[7] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[6] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[5] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[4] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[3] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[2] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[1] = 'x';
//   buf[0] = '0';
//   kputs (buf);
// }

// void
// kputx64 (uint64_t n)
// {
//   char buf[19];
//   buf[18] = 0;
//   buf[17] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[16] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[15] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[14] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[13] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[12] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[11] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[10] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[9] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[8] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[7] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[6] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[5] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[4] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[3] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[2] = to_hex (n & 0xF);
//   n >>= 4;
//   buf[1] = 'x';
//   buf[0] = '0';
//   kputs (buf);
// }

// void
// kputp (const void* p)
// {
// #ifdef ARCH8
//   kputx8 ((uint32_t)p);
// #endif
// #ifdef ARCH16
//   kputx16 ((uint32_t)p);
// #endif
// #ifdef ARCH32
//   kputx32 ((uint32_t)p);
// #endif
// #ifdef ARCH64
//   kputx64 ((uint32_t)p);
// #endif
// }

// void
// clear_console ()
// {
//   /* Pointer to the text framebuffer. */
//   unsigned short* videoram = (unsigned short*)VIDEORAM;

//   unsigned int x, y;
//   for (y = 0; y < HEIGHT; ++y) {
//     for (x = 0; x < WIDTH; ++x) {
//       videoram[y * WIDTH + x] = (BLACK << 12) | (WHITE << 8) | ' ';
//     }
//   }

//   x_location = 0;
//   y_location = 0;
// }

console::console () :
  videoram_ (reinterpret_cast<unsigned short*> (0xB8000)),
  x_location_ (0),
  y_location_ (0)
{
  for (unsigned int y = 0; y < HEIGHT; ++y) {
    for (unsigned int x = 0; x < WIDTH; ++x) {
      videoram_[y * WIDTH + x] = (BLACK << 12) | (WHITE << 8) | ' ';
    }
  }
}

console&
console::operator<< (unsigned int n)
{
  // TODO
  return *this;
}

console&
console::operator<< (console& (*ptr) (console&))
{
  ptr (*this);
  return *this;
}

console&
console::put (char c)
{
  /* Scroll if we are at the bottom of the screen. */
  if (y_location_ == HEIGHT) {
    unsigned int y;
    for (y = 0; y < HEIGHT - 1; ++y) {
      for (unsigned int x = 0; x < WIDTH; ++x) {
	videoram_[y * WIDTH + x] = videoram_[(y + 1) * WIDTH + x];
      }
    }
    /* Fill last line with spaces. */
    for (unsigned int x = 0; x < WIDTH; ++x) {
      videoram_[y * WIDTH + x] = (BLACK << 12) | (WHITE << 8) | ' ';
    }
    --y_location_;
  }
  
  switch (c) {
  case '\b':
    if (x_location_ > 0) {
      --x_location_;
    }
    break;
  case '\t':
    /* A tab is a position divisible by 8. */
    x_location_ = (x_location_ + 8) & ~(8-1);
    break;
  case '\n':
    x_location_ = 0;
    ++y_location_;
    break;
  case '\r':
    x_location_ = 0;
    break;
  default:
    /* Print the character using black on white. */
    videoram_[y_location_ * WIDTH + x_location_] = (BLACK << 12) | (WHITE << 8) | c;
    /* Advance the cursor. */
    ++x_location_;
    if (x_location_ == WIDTH) {
      ++y_location_;
      x_location_ = 0;
    }
  }

  return *this;
}

void
console::flush ()
{
  // Do nothing.  Unbuffered.
}

console&
operator<< (console& c,
	    const char* s)
{
  for (; *s != 0; ++s) {
    c.put (*s);
  }

  return c;
}

console&
endl (console& c)
{
  c.put ('\n');
  c.flush ();
  return c;
}

console kout;
