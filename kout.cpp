#include "kout.hpp"
#include <stdint.h>

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

console::console () :
  videoram_ (reinterpret_cast<unsigned short*> (0xB8000)),
  left_buffer_size_ (0),
  right_buffer_size_ (0),
  x_location_ (0),
  y_location_ (0),
  flags_ (right | dec | skipws),
  width_ (0),
  fill_ (' ')
{
  for (unsigned int y = 0; y < HEIGHT; ++y) {
    for (unsigned int x = 0; x < WIDTH; ++x) {
      videoram_[y * WIDTH + x] = (BLACK << 12) | (WHITE << 8) | ' ';
    }
  }
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

console&
operator<< (console& c,
	    const char* s)
{
  for (; *s != 0; ++s) {
    c.put (*s);
  }

  return c;
}

console kout;
