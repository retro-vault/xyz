#include "xcc_exec_test.h"

static int my_abs(int n) {
    if (n < 0) return -n;
    return n;
}

static int sign(int n) {
    if (n < 0) return -1;
    if (n > 0) return 1;
    return 0;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, my_abs(-7), 7);
    XCC_CHECK_EQ_INT_ID(2, my_abs(7), 7);
    XCC_CHECK_EQ_INT_ID(3, my_abs(0), 0);
    XCC_CHECK_EQ_INT_ID(4, sign(-100), -1);
    XCC_CHECK_EQ_INT_ID(5, sign(0), 0);
    XCC_CHECK_EQ_INT_ID(6, sign(100), 1);
    return 0;
}
