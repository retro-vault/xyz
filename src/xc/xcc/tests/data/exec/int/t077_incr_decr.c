#include "xcc_exec_test.h"

int main(void) {
    int a = 5;
    int b = a++;
    XCC_CHECK_EQ_INT_ID(1, b, 5);
    XCC_CHECK_EQ_INT_ID(2, a, 6);
    int c = ++a;
    XCC_CHECK_EQ_INT_ID(3, c, 7);
    int d = a--;
    XCC_CHECK_EQ_INT_ID(4, d, 7);
    XCC_CHECK_EQ_INT_ID(5, a, 6);
    int e = --a;
    XCC_CHECK_EQ_INT_ID(6, e, 5);
    return 0;
}
