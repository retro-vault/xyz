#include <stdint.h>
#include <stdbool.h>

long long 
_divslonglong (long long numerator, long long denominator)
{
  bool numeratorneg = (numerator < 0);
  bool denominatorneg = (denominator < 0);
  long long d;

  if (numeratorneg)
    numerator = -numerator;
  if (denominatorneg)
    denominator = -denominator;

  d = (unsigned long long)numerator / (unsigned long long)denominator;

  return ((numeratorneg ^ denominatorneg) ? -d : d);
}