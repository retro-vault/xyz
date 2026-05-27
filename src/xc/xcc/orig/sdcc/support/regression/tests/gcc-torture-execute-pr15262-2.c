/*
   pr15262-2.c from the execute part of the gcc torture tests.
 */

#include <testfwk.h>

#ifndef PORT_HOST // Enable when we can do host test in C99 mode

#ifdef __SDCC
#pragma std_c99
#pragma disable_warning 278
#endif

/* PR 15262.  Similar to pr15262-1.c but with no obvious addresses
   being taken in function foo().  Without IPA, by only looking inside
   foo() we cannot tell for certain whether 'q' and 'b' alias each
   other.  */
struct A
{
  int t;
  int i;
};

struct B
{
  int *p;
  float b;
};

float X;

foo (struct B b, struct A *q, float *h)
{
  X += *h;
  *(b.p) = 3;
  q->t = 2;
  return *(b.p);
}
#endif

void
testTortureExecute (void)
{
#ifndef PORT_HOST // Enable when we can do host test in C99 mode
  struct A a;
  struct B b;

  b.p = &a.t;
  if (foo (b, &a, &X) == 3)
    ASSERT (0);

  return;
#endif
}
