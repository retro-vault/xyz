#include "xcc_exec_test.h"

static unsigned char fill_bytes[257];
static unsigned char second_fill[33];

void
fill_constant_bytes(void)
{
    unsigned int i;

    for (i = 0u; i < 257u; ++i)
        fill_bytes[i] = 0xa5u;
}

void
fill_second_object(void)
{
    unsigned int i;

    for (i = 0u; i < 33u; ++i)
        second_fill[i] = 0x3cu;
}

unsigned int
observable_fill_counter(unsigned char reset)
{
    unsigned int i;

    for (i = 0u; i < 5u; ++i)
        second_fill[i] = 0x17u;
    if (reset)
        i = 1u;
    return i;
}

int
main(void)
{
    unsigned int i;

    fill_constant_bytes();
    fill_second_object();
    XCC_CHECK_EQ_UINT_ID(3, fill_bytes[0], 0xa5u);
    XCC_CHECK_EQ_UINT_ID(4, fill_bytes[128], 0xa5u);
    XCC_CHECK_EQ_UINT_ID(5, fill_bytes[256], 0xa5u);
    XCC_CHECK_EQ_UINT_ID(6, second_fill[0], 0x3cu);
    XCC_CHECK_EQ_UINT_ID(7, second_fill[32], 0x3cu);
    for (i = 0u; i < 257u; ++i)
        XCC_CHECK_EQ_UINT_ID(1, fill_bytes[i], 0xa5u);
    for (i = 0u; i < 33u; ++i)
        XCC_CHECK_EQ_UINT_ID(2, second_fill[i], 0x3cu);
    XCC_CHECK_EQ_UINT_ID(8, observable_fill_counter(0u), 5u);
    XCC_CHECK_EQ_UINT_ID(9, observable_fill_counter(1u), 1u);
    return 0;
}
