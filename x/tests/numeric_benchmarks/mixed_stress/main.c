#include "numeric_bench.h"

int main(void) {
    unsigned int i;
    num_t x;
    num_t y;
    num_t half;
    num_t third;
    num_t quarter;

    x = NUM_FROM_INT(2);
    y = NUM_FROM_INT(5);
    half = bench_frac(1, 2);
    third = bench_frac(1, 3);
    quarter = bench_frac(1, 4);

    for (i = 0u; i < 44u; ++i) {
        x = NUM_ADD(NUM_MUL(x, half), NUM_DIV(y, NUM_FROM_INT(3)));
        y = NUM_SUB(NUM_MUL(y, third), NUM_DIV(x, NUM_FROM_INT(4)));
        if (NUM_CMP(y, NUM_FROM_INT(0)) < 0)
            y = NUM_ABS(y);
        x = NUM_ADD(x, quarter);
    }
    return bench_finish(NUM_ADD(x, y), 9u);
}
