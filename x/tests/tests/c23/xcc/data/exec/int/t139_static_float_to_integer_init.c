#include "xcc_exec_test.h"

struct initial_values {
    long whole;
    int negative;
    unsigned char byte;
    _Bool truth;
};

static long scientific = 8e3;
static int truncated = -12.75;
static long expression = 4e3 * 2.0;
static int delayed_truncation = (1.5 + 1.25) * 2.0;
static struct initial_values aggregate = {8e3, -3.9, 255.9, 0.5};
static int array_values[] = {1.9, -2.9, 3e1 / 4.0};

int main(void)
{
    XCC_CHECK_EQ_LONG_ID(1, scientific, 8000L);
    XCC_CHECK_EQ_INT_ID(2, truncated, -12);
    XCC_CHECK_EQ_LONG_ID(3, expression, 8000L);
    XCC_CHECK_EQ_INT_ID(4, delayed_truncation, 5);
    XCC_CHECK_EQ_LONG_ID(5, aggregate.whole, 8000L);
    XCC_CHECK_EQ_INT_ID(6, aggregate.negative, -3);
    XCC_CHECK_EQ_INT_ID(7, aggregate.byte, 255);
    XCC_CHECK_EQ_INT_ID(8, aggregate.truth, 1);
    XCC_CHECK_EQ_INT_ID(9, array_values[0], 1);
    XCC_CHECK_EQ_INT_ID(10, array_values[1], -2);
    XCC_CHECK_EQ_INT_ID(11, array_values[2], 7);
    return 0;
}
