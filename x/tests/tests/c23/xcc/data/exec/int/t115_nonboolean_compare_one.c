#include "xcc_exec_test.h"

static volatile int test_value;

int
classify_not_one(int value)
{
    if (value != 1)
        return 0x1234;
    return 0x5678;
}

int
main(void)
{
    test_value = 2;
    XCC_CHECK_EQ_INT_ID(1, classify_not_one(test_value), 0x1234);
    test_value = 0;
    XCC_CHECK_EQ_INT_ID(2, classify_not_one(test_value), 0x1234);
    test_value = 1;
    XCC_CHECK_EQ_INT_ID(3, classify_not_one(test_value), 0x5678);
    return 0;
}
