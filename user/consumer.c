#include <action.h>
#include <finish.h>
#include <io.h>
#include <stdbool.h>

#define ATTRIBUTE ((0 << 12) | (0xF << 8))
#define HEIGHT 25
#define WIDTH 80

static bool initialized = false;
static unsigned short* video_ram = (unsigned short*)0xB8000;
static unsigned int x_location = 0;
static unsigned int y_location = 0;

static void
put (char c)
{
  // TODO:  Can we scroll with hardware?  Use REP MOVS.
  /* Scroll if we are at the bottom of the screen. */
  if (y_location == HEIGHT) {
    unsigned int y;
    for (y = 0; y < HEIGHT - 1; ++y) {
      for (unsigned int x = 0; x < WIDTH; ++x) {
	video_ram[y * WIDTH + x] = video_ram[(y + 1) * WIDTH + x];
      }
    }
    /* Fill last line with spaces. */
    for (unsigned int x = 0; x < WIDTH; ++x) {
      video_ram[y * WIDTH + x] = ATTRIBUTE | ' ';
    }
    --y_location;
  }
  
  switch (c) {
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
    video_ram[y_location * WIDTH + x_location] = ATTRIBUTE | c;
    /* Advance the cursor. */
    ++x_location;
    if (x_location == WIDTH) {
      ++y_location;
      x_location = 0;
    }
  }
}

static void
printn (const char* s,
	size_t n)
{
  while (n != 0) {
    put (*s++);
    --n;
  }
}

static void
initialize (void)
{
  if (!initialized) {
    // Memory map the video ram.
    // Each element is two bytes corresponding to the character and attribute.
    map (video_ram, video_ram, WIDTH * HEIGHT * 2);
    
    // Clear the screen.
    for (size_t idx = 0; idx < WIDTH * HEIGHT; ++idx) {
      video_ram[idx] = ATTRIBUTE | ' ';
    }
    
    initialized = true;
  }
}

void
init (void)
{
  initialize ();
  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, LILY_ACTION_INIT, init);

void
consume (const void* param,
	 const char* string,
	 size_t length)
{
  initialize ();

  printn (string, length);

  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, 1, consume);
