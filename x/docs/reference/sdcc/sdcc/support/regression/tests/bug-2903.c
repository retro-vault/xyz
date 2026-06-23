/*
   bug-2903.c
   An issue affecting casts of function pointers to void * for pdk.
 */

#include <testfwk.h>

// Declaration before definition needed to trigger bug.
#if !((defined __SDCC_stm8) && defined (__SDCC_MODEL_LARGE)) // STM8 large model has sizeof(void *) != size of function pointers.
void *f (void);

void *
f (void)
{
  return f;
}
#endif

void
testBug (void)
{
#if !((defined __SDCC_stm8) && defined (__SDCC_MODEL_LARGE)) // STM8 large model has sizeof(void *) != size of function pointers.
  ASSERT (f() == f);
#endif
}

