#include <stdint.h>
#include <stdbool.h>

long long 
_modslonglong (long long numerator, long long denominator)
{
  bool numeratorneg = (numerator < 0);
  bool denominatorneg = (denominator < 0);
  long long r;

  if (numeratorneg)
    numerator = -numerator;
  if (denominatorneg)
    denominator = -denominator;

  r = (unsigned long long)numerator % (unsigned long long)denominator;

  return (numeratorneg ? -r : r);
}

