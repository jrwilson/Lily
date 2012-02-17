#include "automaton.h"

static size_t page_size = 0;

size_t
pagesize (void)
{
  if (page_size == 0) {
    page_size = sysconf (SYSCONF_PAGESIZE);
  }
  return page_size;
}

size_t
size_to_pages (size_t size)
{
  size_t ps = pagesize ();
  return ALIGN_UP (size, ps) / ps;
}
