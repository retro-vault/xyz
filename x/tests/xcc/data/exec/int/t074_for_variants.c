#include "xcc_exec_test.h"

static int sum_range(int lo, int hi) {
    int s = 0;
    int i;
    for (i = lo; i <= hi; i = i + 1) {
        s = s + i;
    }
    return s;
}

static int product_range(int lo, int hi) {
    int p = 1;
    int i;
    for (i = lo; i <= hi; i = i + 1) {
        p = p * i;
    }
    return p;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, sum_range(1, 5), 15);
    XCC_CHECK_EQ_INT_ID(2, sum_range(-3, 3), 0);
    XCC_CHECK_EQ_INT_ID(3, product_range(1, 5), 120);
    XCC_CHECK_EQ_INT_ID(4, product_range(2, 4), 24);
    return 0;
}
