#include "numeric_bench.h"

int main(void) {
    unsigned int i;
    num_t x;
    num_t scale;
    num_t bias;
    num_t limit;

    x = NUM_FROM_INT(1);
    scale = bench_frac(3, 2);
    bias = bench_frac(1, 2);
    limit = NUM_FROM_INT(40);
    for (i = 0u; i < 32u; ++i) {
        x = NUM_MUL(NUM_ADD(x, bias), scale);
        if (NUM_CMP(x, limit) > 0)
            x = NUM_DIV(x, NUM_FROM_INT(2));
    }
    return bench_finish(x, 3u);
}
