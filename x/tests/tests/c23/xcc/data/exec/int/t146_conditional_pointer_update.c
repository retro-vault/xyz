#include "xcc_exec_test.h"

static unsigned char flags[16];

__attribute__((noinline)) unsigned
mark_zeroes(unsigned limit)
{
    unsigned count = 0;
    unsigned i;

    for (i = 0; i < limit; ++i) {
        unsigned char *slot = flags + i;

        if (*slot == 0)
            ++count;
        *slot = 1;
    }
    return count;
}

int
main(void)
{
    flags[3] = 1;
    flags[9] = 1;
    XCC_CHECK_EQ_UINT_ID(1, mark_zeroes(12), 10);
    XCC_CHECK_EQ_UINT_ID(2, mark_zeroes(12), 0);
    XCC_CHECK_EQ_UINT_ID(3, flags[12], 0);
    return 0;
}
