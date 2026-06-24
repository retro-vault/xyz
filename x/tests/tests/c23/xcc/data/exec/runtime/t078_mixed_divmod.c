#include "xcc_exec_test.h"

static int check_divisible(int a, int b) {
    return (a % b) == 0;
}

static int next_multiple(int a, int b) {
    int r = a % b;
    if (r == 0) return a;
    return a + (b - r);
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, check_divisible(12, 4), 1);
    XCC_CHECK_EQ_INT_ID(2, check_divisible(13, 4), 0);
    XCC_CHECK_EQ_INT_ID(3, next_multiple(10, 3), 12);
    XCC_CHECK_EQ_INT_ID(4, next_multiple(12, 3), 12);
    XCC_CHECK_EQ_INT_ID(5, next_multiple(1, 10), 10);
    return 0;
}
