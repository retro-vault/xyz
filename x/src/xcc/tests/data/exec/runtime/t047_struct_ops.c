#include "xcc_exec_test.h"

static int pair_sum(int a, int b) {
    return a + b;
}

static int pair_delta(int a, int b) {
    return a - b;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, pair_sum(7, 11), 18);
    XCC_CHECK_EQ_INT_ID(2, pair_delta(21, 8), 13);
    XCC_CHECK_EQ_INT_ID(3, pair_sum(pair_delta(20, 5), 3), 18);
    return 0;
}
