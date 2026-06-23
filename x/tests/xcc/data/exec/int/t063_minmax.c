#include "xcc_exec_test.h"

static int my_min(int a, int b) {
    if (a < b) return a;
    return b;
}

static int my_max(int a, int b) {
    if (a > b) return a;
    return b;
}

static int clamp(int v, int lo, int hi) {
    v = my_max(v, lo);
    v = my_min(v, hi);
    return v;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, my_min(3, 7), 3);
    XCC_CHECK_EQ_INT_ID(2, my_max(-5, 2), 2);
    XCC_CHECK_EQ_INT_ID(3, clamp(-10, 0, 100), 0);
    XCC_CHECK_EQ_INT_ID(4, clamp(50, 0, 100), 50);
    XCC_CHECK_EQ_INT_ID(5, clamp(200, 0, 100), 100);
    return 0;
}
