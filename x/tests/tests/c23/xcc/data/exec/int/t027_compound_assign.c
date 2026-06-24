#include "xcc_exec_test.h"

int main(void) {
    int a = 10;

    a += 5;
    XCC_CHECK_EQ_INT_ID(1, a, 15);

    a *= 3;
    XCC_CHECK_EQ_INT_ID(2, a, 45);

    a >>= 1;
    XCC_CHECK_EQ_INT_ID(3, a, 22);

    a -= 7;
    XCC_CHECK_EQ_INT_ID(4, a, 15);

    a <<= 1;
    XCC_CHECK_EQ_INT_ID(5, a, 30);
    return 0;
}
