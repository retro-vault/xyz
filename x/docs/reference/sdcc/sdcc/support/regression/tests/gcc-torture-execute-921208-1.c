/*
   921208-1.c from the execute part of the gcc torture suite.
 */

#include <testfwk.h>

#ifdef __SDCC
#pragma std_c99
#endif

#if !defined(__SDCC_pdk14) && !defined(__SDCC_pdk15)
double
f(double x) __reentrant
{
  return x*x;
}

double
Int(double (*f)(double) __reentrant, double a) __reentrant
{
  return (*f)(a);
}
#endif

void
testTortureExecute (void)
{
#if !defined(__SDCC_pdk14) && !defined(__SDCC_pdk15)
  if (Int(&f,2.0) != 4.0)
    ASSERT(0);
  return;
#endif
}

