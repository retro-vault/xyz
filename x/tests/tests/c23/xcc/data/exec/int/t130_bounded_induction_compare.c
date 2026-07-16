#include "xcc_exec_test.h"

static unsigned char values[256];

static __attribute__((noinline)) unsigned int
sum_first_200(const unsigned char *cursor)
{
    unsigned int sum = 0;
    int index = 0;

    while (index < 200) {
        sum += *cursor++;
        ++index;
    }
    return sum;
}

static __attribute__((noinline)) unsigned int
sum_first_256(const unsigned char *cursor)
{
    unsigned int sum = 0;
    int index = 0;

    while (index < 256) {
        sum += *cursor++;
        ++index;
    }
    return sum;
}

static __attribute__((noinline)) unsigned int
weighted_first_64(const unsigned char *cursor)
{
    unsigned int sum = 0;
    int index = 0;

    while (index < 64) {
        sum += (unsigned int)cursor[index] * (unsigned int)(index + 1);
        ++index;
    }
    return sum;
}

int
main(void)
{
    unsigned int index;

    for (index = 0; index < 256; ++index)
        values[index] = (unsigned char)(index & 15u);

    XCC_CHECK_EQ_UINT_ID(1, sum_first_200(values), 1468u);
    XCC_CHECK_EQ_UINT_ID(2, sum_first_256(values), 1920u);
    XCC_CHECK_EQ_UINT_ID(3, weighted_first_64(values), 16960u);
    return 0;
}
