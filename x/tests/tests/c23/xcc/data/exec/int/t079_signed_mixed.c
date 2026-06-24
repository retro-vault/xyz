#include "xcc_exec_test.h"

static int sum_squares_diff(int a, int b) {
    int s = a * a - b * b;
    return s / (a - b);
}

static int modular_inverse(int a, int m) {
    int x = 1;
    while ((a * x) % m != 1) {
        x = x + 1;
        if (x >= m) return -1;
    }
    return x;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, sum_squares_diff(5, 3), 8);
    XCC_CHECK_EQ_INT_ID(2, sum_squares_diff(7, 2), 9);
    XCC_CHECK_EQ_INT_ID(3, modular_inverse(3, 7), 5);
    XCC_CHECK_EQ_INT_ID(4, modular_inverse(2, 5), 3);
    return 0;
}
