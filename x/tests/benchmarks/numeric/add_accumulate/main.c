#include "numeric_bench.h"

int main(void) {
    unsigned int i;
    num_t x;
    num_t step;

    x = NUM_FROM_INT(bench_small(1u));
    step = bench_frac(1, 2);
    for (i = 0u; i < 96u; ++i)
        x = NUM_ADD(x, step);
    return bench_finish(x, 1u);
}
