#include <stdbool.h>

#define MSB_SET(x) ((x >> (8*sizeof(x)-1)) & 1)

unsigned long
_divulong (unsigned long x, unsigned long y)
{
  unsigned long reste = 0L;
  unsigned char count = 32;
  bool c;

  do
  {
    // reste: x <- 0;
    c = MSB_SET(x);
    x <<= 1;
    reste <<= 1;
    if (c)
      reste |= 1L;

    if (reste >= y)
    {
      reste -= y;
      // x <- (result = 1)
      x |= 1L;
    }
  }
  while (--count);
  return x;
}