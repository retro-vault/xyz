#include "xcc_exec_test.h"

static volatile int values[] = {-10000, 30000, -30000, 30000};

static int
comparison_mask(int left, int right)
{
    int mask = 0;

    if (left < right)
        mask |= 1;
    if (left <= right)
        mask |= 2;
    if (left > right)
        mask |= 4;
    if (left >= right)
        mask |= 8;
    if (left == right)
        mask |= 16;
    if (left != right)
        mask |= 32;
    return mask;
}

int
main(void)
{
    int low = values[0];
    int high = values[1];

    XCC_CHECK_EQ_INT_ID(1, comparison_mask(low, high), 35);
    XCC_CHECK_EQ_INT_ID(2, comparison_mask(high, low), 44);
    XCC_CHECK_EQ_INT_ID(3, comparison_mask(values[2], values[3]), 35);
    XCC_CHECK_EQ_INT_ID(4, comparison_mask(values[3], values[2]), 44);
    XCC_CHECK_EQ_INT_ID(5, comparison_mask(values[2], values[2]), 26);
    XCC_CHECK_EQ_INT_ID(6, low < high, 1);
    XCC_CHECK_EQ_INT_ID(7, low <= high, 1);
    XCC_CHECK_EQ_INT_ID(8, high > low, 1);
    XCC_CHECK_EQ_INT_ID(9, high >= low, 1);
    return 0;
}
