#include "xcc_exec_test.h"

int main(void) {
    int a = 16;
    XCC_CHECK_EQ_INT_ID(1, a >> 1, 8);
    XCC_CHECK_EQ_INT_ID(2, a >> 2, 4);
    int b = 1;
    XCC_CHECK_EQ_INT_ID(3, b << 4, 16);
    XCC_CHECK_EQ_INT_ID(4, b << 6, 64);
    int c = -32;
    XCC_CHECK_EQ_INT_ID(5, c >> 1, -16);
    return 0;
}
