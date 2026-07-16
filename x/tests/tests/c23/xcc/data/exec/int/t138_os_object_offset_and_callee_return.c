#include "xcc_exec_test.h"

struct result_pair {
    unsigned first;
    unsigned second;
};

static volatile struct result_pair stored;
static volatile unsigned folded;

__attribute__((noinline)) void
store_pair(unsigned first, unsigned second)
{
    stored.first = first;
    stored.second = second;
}

__attribute__((noinline)) void
fold_four(unsigned a, unsigned b, unsigned c, unsigned d)
{
    folded = (unsigned)(a + b + c + d);
}

int
main(void)
{
    store_pair(0x1234u, 0xabcdU);
    XCC_CHECK_EQ_UINT_ID(1, stored.first, 0x1234u);
    XCC_CHECK_EQ_UINT_ID(2, stored.second, 0xabcdU);

    fold_four(1u, 20u, 300u, 4000u);
    XCC_CHECK_EQ_UINT_ID(3, folded, 4321u);
    return 0;
}
