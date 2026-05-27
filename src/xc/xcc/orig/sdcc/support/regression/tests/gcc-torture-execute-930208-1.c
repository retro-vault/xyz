/*
   930208-1.c from the execute part of the gcc torture suite.
 */

#include <testfwk.h>

#ifndef PORT_HOST // Enable when we can do host test in C99 mode

#ifdef __SDCC
#pragma std_c99
#pragma disable_warning 278
#endif

typedef union {
  long l;
  struct { char b3, b2, b1, b0; } c;
} T;

f (T u)
{
  ++u.c.b0;
  ++u.c.b3;
  return (u.c.b1 != 2 || u.c.b2 != 2);
}
#endif

void
testTortureExecute (void)
{
#ifndef PORT_HOST // Enable when we can do host test in C99 mode
  T u;
  u.c.b1 = 2;
  u.c.b2 = 2;
  u.c.b0 = ~0;
  u.c.b3 = ~0;
  if (f (u))
    ASSERT(0);
  return;
#endif
}

