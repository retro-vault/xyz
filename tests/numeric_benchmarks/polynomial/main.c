#include "numeric_bench.h"

int main(void) {
    unsigned int i;
    num_t x;
    num_t y;

    x = bench_frac(3, 2);
    y = NUM_FROM_INT(0);
    for (i = 0u; i < 36u; ++i) {
        y = NUM_FROM_INT(2);
        y = NUM_ADD(NUM_MUL(y, x), NUM_FROM_INT(3));
        y = NUM_SUB(NUM_MUL(y, x), NUM_FROM_INT(1));
        x = NUM_ADD(x, bench_frac(1, 8));
        if (NUM_CMP(x, NUM_FROM_INT(6)) > 0)
            x = bench_frac(3, 2);
    }
    return bench_finish(y, 7u);
}
