#include "numeric_bench.h"

int main(void) {
    unsigned int i;
    num_t x;
    num_t divisor;
    num_t bias;
    num_t limit;

    x = NUM_FROM_INT(90);
    divisor = NUM_FROM_INT(3);
    bias = NUM_FROM_INT(7);
    limit = NUM_FROM_INT(60);
    for (i = 0u; i < 40u; ++i) {
        x = NUM_ADD(NUM_DIV(x, divisor), bias);
        if (NUM_CMP(x, limit) > 0)
            x = NUM_SUB(x, NUM_FROM_INT(25));
    }
    return bench_finish(x, 4u);
}
