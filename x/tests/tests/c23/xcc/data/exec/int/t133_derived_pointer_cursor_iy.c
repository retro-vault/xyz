#include "xcc_exec_test.h"

static __attribute__((noinline)) unsigned int
sum_window(const unsigned int *base, unsigned int start, unsigned int count)
{
    const unsigned int *cursor = base + start;
    unsigned int sum = 0;

    while (count-- != 0) {
        sum += *cursor;
        ++cursor;
    }
    return sum;
}

int
main(void)
{
    static const unsigned int values[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
    };

    XCC_CHECK_EQ_UINT_ID(1, sum_window(values, 3, 5), 30u);
    XCC_CHECK_EQ_UINT_ID(2, sum_window(values, 0, 12), 78u);
    return 0;
}
