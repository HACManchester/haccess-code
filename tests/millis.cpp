/* millis.cpp
 *
 * Simple millis() emulation for the test suite
 * Copyright 2016 Ben Dooks
*/

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <time.h>
#include "millis.h"

unsigned long millis(void)
{
  struct timespec ts;
  int ret;

  ret = clock_gettime(CLOCK_REALTIME_COARSE, &ts);
  if (ret != 0) {
    perror("clock_gettime");
    exit(5);
  }

  return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}
