#include "xcc_exec_test.h"

static void
advance_pair(int quotient, int previous[2])
{
    int next = previous[1] - previous[0] * quotient;

    previous[1] = previous[0];
    previous[0] = next;
}

int
main(void)
{
    int pair[2] = {2, 7};

    advance_pair(3, pair);
    XCC_CHECK_EQ_INT_ID(1, pair[0], 1);
    XCC_CHECK_EQ_INT_ID(2, pair[1], 2);
    advance_pair(4, pair);
    XCC_CHECK_EQ_INT_ID(3, pair[0], -2);
    XCC_CHECK_EQ_INT_ID(4, pair[1], 1);
    return 0;
}
