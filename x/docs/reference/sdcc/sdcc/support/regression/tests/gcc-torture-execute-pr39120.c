/*
   pr39120.c from the execute part of the gcc torture tests.
 */

#include <testfwk.h>

#ifdef __SDCC
#pragma std_c99
#endif

struct X { int *p; } x;

struct X
foo(int *p) { struct X x; x.p = p; return x; }

void
bar() { *x.p = 1; }

void
testTortureExecute (void)
{
  int i = 0;
  x = foo(&i);
  bar();
  if (i != 1)
    ASSERT (0);
  return;
}
