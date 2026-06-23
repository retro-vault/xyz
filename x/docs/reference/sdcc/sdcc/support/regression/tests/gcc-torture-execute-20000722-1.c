/*
   20000722-1.c from the execute part of the gcc torture tests.
 */

#include <testfwk.h>

#ifdef __SDCC
#pragma std_c99
#pragma disable_warning 360
#endif

struct s { char *p; int t; };

extern void bar (void);
extern void foo (struct s *);

void
testTortureExecute (void)
{

  bar ();
  bar ();
  return;
}

void 
bar (void)
{
  foo (& (struct s) { "hi", 1 });
}

void foo (struct s *p)
{
  if (p->t != 1)
    ASSERT (0);
  p->t = 2;
}

