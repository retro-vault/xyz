#include "xcc_exec_test.h"

static int in_range(int v, int lo, int hi) {
    if (v < lo) return 0;
    if (v > hi) return 0;
    return 1;
}

static int either(int a, int b) {
    if (a != 0) return 1;
    if (b != 0) return 1;
    return 0;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, in_range(5, 1, 10), 1);
    XCC_CHECK_EQ_INT_ID(2, in_range(0, 1, 10), 0);
    XCC_CHECK_EQ_INT_ID(3, in_range(11, 1, 10), 0);
    XCC_CHECK_EQ_INT_ID(4, either(0, 0), 0);
    XCC_CHECK_EQ_INT_ID(5, either(1, 0), 1);
    XCC_CHECK_EQ_INT_ID(6, either(0, 1), 1);
    return 0;
}
