#include "xcc_exec_test.h"

int main(void) {
    int a = 10;
    a += 5;
    XCC_CHECK_EQ_INT_ID(1, a, 15);
    a -= 3;
    XCC_CHECK_EQ_INT_ID(2, a, 12);
    a *= 2;
    XCC_CHECK_EQ_INT_ID(3, a, 24);
    a /= 4;
    XCC_CHECK_EQ_INT_ID(4, a, 6);
    a %= 4;
    XCC_CHECK_EQ_INT_ID(5, a, 2);
    a <<= 3;
    XCC_CHECK_EQ_INT_ID(6, a, 16);
    a >>= 2;
    XCC_CHECK_EQ_INT_ID(7, a, 4);
    return 0;
}
