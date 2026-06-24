#include "xcc_exec_test.h"

int main(void) {
    int a = -100;
    int b = -7;
    XCC_CHECK_EQ_INT_ID(1, a / b, 14);
    XCC_CHECK_EQ_INT_ID(2, a % b, -2);
    int c = 100;
    int d = -7;
    XCC_CHECK_EQ_INT_ID(3, c / d, -14);
    XCC_CHECK_EQ_INT_ID(4, c % d, 2);
    int e = -1;
    int f = 1;
    XCC_CHECK_EQ_INT_ID(5, e / f, -1);
    XCC_CHECK_EQ_INT_ID(6, e % f, 0);
    return 0;
}
