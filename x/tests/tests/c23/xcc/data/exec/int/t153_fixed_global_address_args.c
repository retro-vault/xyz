#include "xcc_exec_test.h"

static unsigned char values[32];

static __attribute__((noinline)) unsigned int
sum_range(unsigned char *cursor, unsigned char *end)
{
    unsigned int sum = 0;

    while (cursor < end)
        sum += *cursor++;
    return sum;
}

int
main(void)
{
    unsigned int i;

    for (i = 0; i < sizeof(values); ++i)
        values[i] = (unsigned char)(i + 1);

    XCC_CHECK_EQ_UINT_ID(1, sum_range(values + 3, values + 19), 184);
    return 0;
}
