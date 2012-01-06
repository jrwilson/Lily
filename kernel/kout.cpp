#include "kout.hpp"

#include "vm.hpp"

char console::left_buffer_[3];
size_t console::left_buffer_size_ = 0;

char console::right_buffer_[20];
size_t console::right_buffer_size_ = 0;

unsigned int console::x_location_ = 0;
unsigned int console::y_location_ = 0;
console::fmtflags console::flags_ = 0;
console::streamsize console::width_ = 0;
char console::fill_ = 0;

console&
console::put (char c)
{
  // Because the video ram is not mapped in all page directories.
  physical_address_t old = vm::switch_to_directory (vm::get_kernel_page_directory_physical_address ());

  // TODO:  Can we scroll with hardware?
  /* Scroll if we are at the bottom of the screen. */
  if (y_location_ == HEIGHT) {
    unsigned int y;
    for (y = 0; y < HEIGHT - 1; ++y) {
      for (unsigned int x = 0; x < WIDTH; ++x) {
	videoram ()[y * WIDTH + x] = videoram ()[(y + 1) * WIDTH + x];
      }
    }
    /* Fill last line with spaces. */
    for (unsigned int x = 0; x < WIDTH; ++x) {
      videoram ()[y * WIDTH + x] = (BLACK << 12) | (WHITE << 8) | ' ';
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
    videoram ()[y_location_ * WIDTH + x_location_] = (BLACK << 12) | (WHITE << 8) | c;
    /* Advance the cursor. */
    ++x_location_;
    if (x_location_ == WIDTH) {
      ++y_location_;
      x_location_ = 0;
    }
  }

  vm::switch_to_directory (old);
  return *this;
}

console kout;
