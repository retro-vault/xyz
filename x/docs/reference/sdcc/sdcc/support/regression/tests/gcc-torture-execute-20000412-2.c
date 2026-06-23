/*
   20000412-2.c from the execute part of the gcc torture tests.
 */

#include <testfwk.h>

#ifdef __SDCC
#pragma std_c99
#endif

int f(int a,int *y) __reentrant
{
  int x = a;

  if (a==0)
    return *y;

  return f(a-1,&x);
}

void
testTortureExecute (void)
{
#if !defined(__SDCC_pic16) && !defined(__SDCC_pic14) // Unsupported reentrancy
  if (f (10, (int *) 0) != 1)
    ASSERT (0);
  return;
#endif
}

