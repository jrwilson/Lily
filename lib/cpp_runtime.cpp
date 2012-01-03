#include <stddef.h>

int
frobnicate (void)
{
  return 3;
}

void*
operator new (size_t)
{
  // TODO
  asm ("hlt\n");
  return 0;
}

// void
// operator delete (void*)
// {
//   // TODO
//   asm ("hlt\n");
// }
