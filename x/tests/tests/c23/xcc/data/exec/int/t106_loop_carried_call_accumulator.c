#include "xcc_exec_test.h"

static volatile unsigned char sequence_value;

unsigned char
next_sequence_value(void)
{
    sequence_value = (unsigned char)(sequence_value + 3u);
    return sequence_value;
}

unsigned char
sum_sequence(void)
{
    unsigned char sum = 0u;
    unsigned int i;

    for (i = 0u; i < 4u; ++i)
        sum = (unsigned char)(sum + next_sequence_value());
    return sum;
}

int
main(void)
{
    sequence_value = 0u;
    XCC_CHECK_EQ_UINT_ID(1, sum_sequence(), 30u);
    return 0;
}
