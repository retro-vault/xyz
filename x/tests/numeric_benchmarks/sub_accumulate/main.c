#include "numeric_bench.h"

int main(void) {
    unsigned int i;
    num_t x;
    num_t step;

    x = NUM_FROM_INT(80);
    step = bench_frac(1, 3);
    for (i = 0u; i < 96u; ++i)
        x = NUM_SUB(x, step);
    return bench_finish(x, 2u);
}
