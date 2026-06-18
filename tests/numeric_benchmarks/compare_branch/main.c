#include "numeric_bench.h"

int main(void) {
    unsigned int i;
    num_t x;
    num_t lo;
    num_t hi;
    num_t delta;

    x = NUM_FROM_INT(bench_small(8u));
    lo = NUM_FROM_INT(2);
    hi = NUM_FROM_INT(9);
    delta = bench_frac(1, 2);
    for (i = 0u; i < 104u; ++i) {
        if (NUM_CMP(x, hi) > 0)
            x = NUM_SUB(x, NUM_FROM_INT(5));
        else if (NUM_CMP(x, lo) < 0)
            x = NUM_ADD(x, NUM_FROM_INT(3));
        else
            x = NUM_ADD(x, delta);
    }
    return bench_finish(x, 8u);
}
