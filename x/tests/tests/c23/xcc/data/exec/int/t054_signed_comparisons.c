#include "xcc_exec_test.h"

int main(void) {
    int a = -5;
    int b = 3;
    XCC_CHECK_EQ_INT_ID(1, a < b, 1);
    XCC_CHECK_EQ_INT_ID(2, a > b, 0);
    XCC_CHECK_EQ_INT_ID(3, a <= -5, 1);
    XCC_CHECK_EQ_INT_ID(4, a >= 0, 0);
    XCC_CHECK_EQ_INT_ID(5, a == -5, 1);
    XCC_CHECK_EQ_INT_ID(6, a != b, 1);
    return 0;
}
