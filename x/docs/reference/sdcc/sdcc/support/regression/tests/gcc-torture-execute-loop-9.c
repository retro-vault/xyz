/*
   loop-9.c from the execute part of the gcc torture tests.
 */

#include <testfwk.h>

#ifndef PORT_HOST // Enable when we can do host test in C99 mode

#ifdef __SDCC
#pragma std_c89
#endif

/* Source: Neil Booth, from PR # 115.  */

int false()
{
  return 0;
}
#endif

void
testTortureExecute (void)
{
#ifndef PORT_HOST // Enable when we can do host test in C99 mode
  int count = 0;

  while (false() || count < -123)
    ++count;

  if (count)
    ASSERT (0);

  return;
#endif
}
