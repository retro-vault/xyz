#include "xcc_exec_test.h"

int main(void) {
    int a = -7;
    int b = 3;
    int c = -a;
    int d = +(b * 4);
    int e = d + 2;

    XCC_CHECK_EQ_INT_ID(1, c, 7);
    XCC_CHECK_EQ_INT_ID(2, d, 12);
    XCC_CHECK_EQ_INT_ID(3, e, 14);
    return 0;
}
