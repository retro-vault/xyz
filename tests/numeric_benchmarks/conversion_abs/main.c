#include "numeric_bench.h"

int main(void) {
    unsigned int i;
    unsigned int acc;
    num_t x;
    int v;

    acc = 0x1357u;
    for (i = 0u; i < 120u; ++i) {
        v = (int)((i & 15u) - 8u);
        x = NUM_ABS(NUM_FROM_INT(v));
        acc = bench_mix(acc, NUM_TO_INT(x));
    }
    return (int)(acc & 0x7fffu);
}
