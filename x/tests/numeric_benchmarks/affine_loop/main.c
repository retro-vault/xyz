#include "numeric_bench.h"

int main(void) {
    unsigned int i;
    num_t x;
    num_t a;
    num_t b;
    num_t limit;

    x = NUM_FROM_INT(2);
    a = bench_frac(5, 4);
    b = bench_frac(3, 4);
    limit = NUM_FROM_INT(50);
    for (i = 0u; i < 48u; ++i) {
        x = NUM_ADD(NUM_MUL(x, a), b);
        if (NUM_CMP(x, limit) > 0)
            x = NUM_SUB(x, NUM_FROM_INT(31));
    }
    return bench_finish(x, 6u);
}
