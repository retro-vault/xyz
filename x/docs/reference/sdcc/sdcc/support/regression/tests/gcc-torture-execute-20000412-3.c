/*
   20000412-3.c from the execute part of the gcc torture tests.
 */

#include <testfwk.h>

#ifdef __SDCC
#pragma std_c99
#endif

typedef struct {
  char y;
  char x[32];
} X;

int f(X x, X y);

int z (void)
{
  X xxx;
  xxx.x[0] =
  xxx.x[31] = '0';
  xxx.y = 0xf;
  return f (xxx, xxx);
}

void
testTortureExecute (void)
{
  int val;

  val = z ();
  if (val != 0x60)
    ASSERT (0);
  return;
}

int f(X x, X y)
{
  if (x.y != y.y)
    return 'F';

  return x.x[0] + y.x[0];
}

