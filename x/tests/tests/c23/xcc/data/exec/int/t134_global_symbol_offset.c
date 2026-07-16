#include "xcc_exec_test.h"

static unsigned char bytes[20];

static __attribute__((noinline)) unsigned int
fill_and_sum(void)
{
    unsigned char *cursor = bytes;
    unsigned int value = 1;
    unsigned int sum = 0;

    while (cursor < bytes + 20) {
        *cursor++ = (unsigned char)value;
        sum += value;
        ++value;
    }
    return sum;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, fill_and_sum(), 210u);
    XCC_CHECK_EQ_UINT_ID(2, bytes[0], 1u);
    XCC_CHECK_EQ_UINT_ID(3, bytes[19], 20u);
    return 0;
}
