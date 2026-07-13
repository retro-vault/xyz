#include "xcc_exec_test.h"

static volatile unsigned int loop_sink;

unsigned int
run_reused_signed_counter(void)
{
    int i;
    unsigned int sum = 0u;

    for (i = 0; i < 260; ++i) {
        if (i == 259)
            ++sum;
    }
    for (i = 0; i < 7; ++i)
        sum = (unsigned int)(sum + (unsigned int)i);
    return sum;
}

unsigned int
run_dynamic_byte_bound(unsigned char selector)
{
    unsigned int i;
    unsigned int limit = (unsigned int)((selector & 15u) + 1u);

    loop_sink = 0u;
    for (i = 0; i < limit; ++i)
        ++loop_sink;
    return loop_sink;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, run_reused_signed_counter(), 22u);
    XCC_CHECK_EQ_UINT_ID(2, run_dynamic_byte_bound(0u), 1u);
    XCC_CHECK_EQ_UINT_ID(3, run_dynamic_byte_bound(15u), 16u);
    XCC_CHECK_EQ_UINT_ID(4, run_dynamic_byte_bound(0xffu), 16u);
    return 0;
}
