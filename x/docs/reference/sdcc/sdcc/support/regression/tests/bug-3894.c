/*
   bug-3256.c
   A bug in a machine-independnet optimization triggered an assertion in the compiler.
 */

#include <testfwk.h>

// Test needs flat 16-bit address space.
#if defined(__SDCC_stm8) || defined(__SDCC_f8) || defined(__SDCC_f8l) || defined(__SDCC_z80) || defined(__SDCC_z80n) || defined(__SDCC_z180)
#pragma disable_warning 88
typedef struct {
   union {
     char A;
     char B;
   } U;
} S;

void f(void) {
   ((S *)0x5420)->U.A = 2;
}
#endif

void
testBug(void)
{
#if defined(__SDCC_stm8) // Only stm8 has the peripherals at the correct memory location
  f();
#endif
}



