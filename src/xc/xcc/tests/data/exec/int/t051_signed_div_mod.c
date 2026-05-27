#include "xcc_exec_test.h"

int main(void) {
    int a = -17;
    int b = 5;
    XCC_CHECK_EQ_INT_ID(1, a / b, -3);
    XCC_CHECK_EQ_INT_ID(2, a % b, -2);
    int c = 17;
    int d = -5;
    XCC_CHECK_EQ_INT_ID(3, c / d, -3);
    XCC_CHECK_EQ_INT_ID(4, c % d, 2);
    int e = -1234;
    int f = 17;
    XCC_CHECK_EQ_INT_ID(5, e / f, -72);
    XCC_CHECK_EQ_INT_ID(6, e % f, -10);
    return 0;
}
