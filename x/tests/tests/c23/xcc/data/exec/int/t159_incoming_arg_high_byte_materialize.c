#include "xcc_exec_test.h"

static unsigned char
survives_high_byte_first_use(int value)
{
    int remaining;

    if (value < 0)
        return 0;
    remaining = value;
    while (remaining != 0)
        remaining /= 10;
    return value == 145;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, survives_high_byte_first_use(145), 1u);
    XCC_CHECK_EQ_UINT_ID(2, survives_high_byte_first_use(543), 0u);
    XCC_CHECK_EQ_UINT_ID(3, survives_high_byte_first_use(-1), 0u);
    return 0;
}
