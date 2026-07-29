#include "xcc_exec_test.h"

static unsigned tally;

__attribute__((noinline)) void
add_to_tally(unsigned value)
{
    tally += value;
}

__attribute__((noinline)) unsigned
run_one_trip(unsigned value)
{
    unsigned i;

    for (i = 0; i < 1; ++i)
        add_to_tally(value);
    return tally;
}

__attribute__((noinline)) unsigned
sum_narrowed_loop(void)
{
    unsigned i;

    tally = 0u;
    for (i = 0u; i < 32u; ++i)
        add_to_tally(i);
    return tally;
}

__attribute__((noinline)) unsigned
combine_four(unsigned first, unsigned second, unsigned third, unsigned fourth)
{
    return first + second + third + fourth;
}

__attribute__((noinline)) unsigned
compare_and_send(unsigned value)
{
    return combine_four(value == 7u, 10u, 20u, 30u);
}

int
main(void)
{
    tally = 5u;
    XCC_CHECK_EQ_UINT_ID(1, run_one_trip(8u), 13u);
    XCC_CHECK_EQ_UINT_ID(2, compare_and_send(7u), 61u);
    XCC_CHECK_EQ_UINT_ID(3, compare_and_send(8u), 60u);
    XCC_CHECK_EQ_UINT_ID(4, sum_narrowed_loop(), 496u);
    return 0;
}
