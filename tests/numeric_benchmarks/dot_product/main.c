#include "numeric_bench.h"

int main(void) {
    unsigned int i;
    num_t acc;
    num_t a;
    num_t b;
    num_t limit;

    acc = NUM_FROM_INT(0);
    limit = NUM_FROM_INT(60);
    for (i = 1u; i <= 40u; ++i) {
        a = NUM_FROM_INT((int)((i & 7u) + 1u));
        b = NUM_FROM_INT((int)(((i * 3u) & 7u) + 1u));
        acc = NUM_ADD(acc, NUM_MUL(a, b));
        if (NUM_CMP(acc, limit) > 0)
            acc = NUM_SUB(acc, NUM_FROM_INT(37));
    }
    return bench_finish(acc, 5u);
}
