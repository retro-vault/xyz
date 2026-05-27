/*
   930513-1.c from the execute part of the gcc torture suite.
 */

#include <testfwk.h>

#ifndef PORT_HOST // Enable when we can do host test in C99 mode
#pragma disable_warning 278

/* { dg-additional-options "-Wl,-u,_printf_float" { target newlib_nano_io } } */

#include <stdio.h>
char buf[2];

f (fp)
     int (*fp)(char *, const char *, ...);
{
  (*fp)(buf, "%.0f", 5.0);
}
#endif

void
testTortureExecute (void)
{
#if 0 // TODO: Check why this fails!
  f (&sprintf);
  if (buf[0] != '5' || buf[1] != 0)
    ASSERT (0);
  return;
#endif
}

