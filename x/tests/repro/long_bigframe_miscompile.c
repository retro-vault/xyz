// Minimal reproducer for the t127/t128-131 miscompiles.
//
// Symptom: a 4-byte value (long OR float — anything 32-bit) stored into an
// array element in a function with a LARGE stack frame (>~128 bytes, which
// forces the deep-frame IX/SP-relative addressing path) gets its HIGH word
// corrupted.  Fails at -O0 through -O3.  Small frames (array fits in IX
// displacement) and scalar destinations are unaffected.  Taking &v (forcing
// the value to a stable slot) hides it — hence printf-based tests can mask it.
//
// Build:  xcc --platform=emu --oformat=binary this.c -o a.bin
// Run:    xemu --run --load-bin a.bin --emu-stdio --max-steps 30000000
//
// Expected: W0=000003e8 W63=0000fa00
// Actual:   W0=fc1803e8 W63=fc18fa00   (high word = stale frame slot)
#include <stdio.h>
typedef unsigned long u32;
int main(void){
    u32 W[64];                                  // 256-byte frame -> deep offsets
    for (int i=0;i<64;i++) W[i] = (u32)(i+1) * 1000UL;
    printf("W0=%08lx W63=%08lx (exp 000003e8 0000fa00)\n", W[0], W[63]);
    return 0;
}
