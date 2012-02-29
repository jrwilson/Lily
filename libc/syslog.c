#include "automaton.h"
#include "string.h"

int
syslog (const char* msg)
{
  return syslogn (msg, strlen (msg));
}
