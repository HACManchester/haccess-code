/* small avr compatible c library wrapper to linux c library
 *
 * Copyright 2016 Ben Dooks <ben@fluff.org>
 */

#include <stdio.h>
#include <stdlib.h>

#include "avr/clib.h"

char *itoa (int val, char *s, int radix)
{
  return ltoa(val, s, radix);
}

char *ltoa (long val, char *s, int radix)
{
  switch (radix) {
  case 10:
    sprintf(s, "%ld", val);
    return s;
  case 16:
    sprintf(s, "%lx", val);
    return s;
  }
  
  fprintf(stderr, "%s: radix %d not supported\n", __func__, radix);
  return NULL;
}

char *utoa (unsigned int val, char *s, int radix)
{
  return ultoa(val, s, radix);
}

char *ultoa (unsigned long val, char *s, int radix)
{
  switch (radix) {
  case 10:
    sprintf(s, "%lu", val);
    return s;
  case 16:
    sprintf(s, "%lx", val);
    return s;
  }

  fprintf(stderr, "%s: radix %d not supported\n", __func__, radix);
  return NULL;
}

char *dtostrf (double val, signed char width, unsigned char prec, char *s)

{
  sprintf(s, "%*f", width, val);
  return s;
}

