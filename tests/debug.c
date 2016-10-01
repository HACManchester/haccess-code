
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void __debug_printf(const char *msg, ...)
{
  struct timespec ts;
  va_list va;
  
  va_start(va, msg);
 
  clock_gettime(CLOCK_MONOTONIC, &ts);
  printf("[%ld.%ld] ", ts.tv_sec, ts.tv_nsec / 1000);

  vprintf(msg, va);
  va_end(va);
}
