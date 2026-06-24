#include "xcc_exec_test.h"

static int complex_expr(int a, int b, int c, int d) {
    return (((a * b + c) * (d - 2) + (a - b) * c + 42) << 1) | (a & b);
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, complex_expr(3, 4, 5, 8), 278);
    XCC_CHECK_EQ_INT_ID(2, complex_expr(7, 2, 3, 9), 354);
    XCC_CHECK_EQ_INT_ID(3, complex_expr(9, 6, 4, 5), 456);
    XCC_CHECK_EQ_INT_ID(4, complex_expr(-3, 5, 2, 7), -73);
    return 0;
}
