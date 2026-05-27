/*
inst-check.c from the execute part of the gcc torture tests.
*/

#include <testfwk.h>

#ifndef PORT_HOST // Enable when we can do host test in C99 mode

#ifdef __SDCC
#pragma std_c99
#pragma disable_warning 278
#endif

#include <stdarg.h>

f(m)
{
  int i,s=0;
  for(i=0;i<m;i++)
    s+=i;
  return s;
}
#endif

void
testTortureExecute (void)
{
  return;
}
