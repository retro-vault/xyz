/** uabs.c
*/

#include <testfwk.h>
#undef __STDC_VERSION__
#define __STDC_VERSION__ 202500L // Just something bigger than the C23 value for now. Will need to change this once C2y has an official value (and thus is no longer C2y).

#pragma std_c23

#include <stdlib.h>
#include <inttypes.h>

void
testAbs(void)
{
#ifdef __SDCC
  ASSERT (uabs(0x7fff) == 0x7fffu);
  ASSERT (uabs(-1000)  == 1000u);
  ASSERT (uabs(-32768) == 32768u);

  ASSERT (ulabs(0x7fffffffl) == 0x7ffffffful);
  ASSERT (ulabs(-1000000l)    == 1000000ul);
  ASSERT (ulabs(-2147483648l) == 2147483648ul);

  ASSERT (ullabs(0x7fffll) == 0x7fffllu);
  ASSERT (ullabs(-1000ll)  == 1000llu);
  ASSERT (ullabs(-9223372036854775808ll) == 9223372036854775808ull);

  ASSERT (umaxabs(0x7FFFffffll) == 0x7FFFffffull);
  ASSERT (umaxabs(-1000000ll)    == 1000000ull);
  ASSERT (umaxabs(-9223372036854775808ll) == 9223372036854775808ull);
#endif
}

