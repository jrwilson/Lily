#include "kput.h"

#include "memory.h"

#define VIDEORAM (KERNEL_OFFSET + 0xB8000)

/* Width and height of the framebuffer. */
#define WIDTH 80
#define HEIGHT 25

/* Colors. */
#define BLACK 0
#define BLUE 1 
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define LIGHT_GREY 7
#define DARK_GREY 8
#define LIGHT_BLUE 9
#define LIGHT_GREEN 10
#define LIGHT_CYAN 11
#define LIGHT_RED 12
#define LIGHT_MAGENTA 13
#define LIGHT_BROWN 14
#define WHITE 15

/* x and y location of the cursor. */
static unsigned int x_location = 0;
static unsigned int y_location = 0;

void
kputs (char* string)
{
  /* Pointer to the text framebuffer. */
  unsigned short* videoram = (unsigned short*)VIDEORAM;

  for (; *string != 0; ++string) {
    /* Scroll if we are at the bottom of the screen. */
    if (y_location == HEIGHT) {
      unsigned int x, y;
      for (y = 0; y < HEIGHT - 1; ++y) {
	for (x = 0; x < WIDTH; ++x) {
	  videoram[y * WIDTH + x] = videoram[(y + 1) * WIDTH + x];
	}
      }
      /* Fill last line with spaces. */
      for (x = 0; x < WIDTH; ++x) {
	videoram[y * WIDTH + x] = (BLACK << 12) | (WHITE << 8) | ' ';
      }
      --y_location;
    }

    switch (*string) {
    case '\b':
      if (x_location > 0) {
	--x_location;
      }
      break;
    case '\t':
      /* A tab is a position divisible by 8. */
      x_location = (x_location + 8) & ~(8-1);
      break;
    case '\n':
      x_location = 0;
      ++y_location;
      break;
    case '\r':
      x_location = 0;
      break;
    default:
      /* Print the character using black on white. */
      videoram[y_location * WIDTH + x_location] = (BLACK << 12) | (WHITE << 8) | *string;
      /* Advance the cursor. */
      ++x_location;
      if (x_location == WIDTH) {
	++y_location;
	x_location = 0;
      }
    }
  }
}

static char
to_hex (unsigned int n)
{
  if (n < 10) {
    return n + '0';
  }
  else {
    return n - 10 + 'A';
  }
}

void
kputucx (unsigned char n)
{
  char buf[5];
  buf[4] = 0;
  buf[3] = to_hex (n & 0xF);
  n >>= 4;
  buf[2] = to_hex (n & 0xF);
  n >>= 4;
  buf[1] = 'x';
  buf[0] = '0';
  kputs (buf);
}

void
kputuix (unsigned int n)
{
  char buf[11];
  buf[10] = 0;
  buf[9] = to_hex (n & 0xF);
  n >>= 4;
  buf[8] = to_hex (n & 0xF);
  n >>= 4;
  buf[7] = to_hex (n & 0xF);
  n >>= 4;
  buf[6] = to_hex (n & 0xF);
  n >>= 4;
  buf[5] = to_hex (n & 0xF);
  n >>= 4;
  buf[4] = to_hex (n & 0xF);
  n >>= 4;
  buf[3] = to_hex (n & 0xF);
  n >>= 4;
  buf[2] = to_hex (n & 0xF);
  n >>= 4;
  buf[1] = 'x';
  buf[0] = '0';
  kputs (buf);
}

void
clear_console ()
{
  /* Pointer to the text framebuffer. */
  unsigned short* videoram = (unsigned short*)VIDEORAM;

  unsigned int x, y;
  for (y = 0; y < HEIGHT; ++y) {
    for (x = 0; x < WIDTH; ++x) {
      videoram[y * WIDTH + x] = (BLACK << 12) | (WHITE << 8) | ' ';
    }
  }

  x_location = 0;
  y_location = 0;
}
