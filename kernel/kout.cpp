#include "kout.hpp"

char console::left_buffer_[3];
size_t console::left_buffer_size_ = 0;

char console::right_buffer_[20];
size_t console::right_buffer_size_ = 0;

unsigned int console::x_location_ = 0;
unsigned int console::y_location_ = 0;
console::fmtflags console::flags_ = 0;
console::streamsize console::width_ = 0;
char console::fill_ = 0;

console kout;
