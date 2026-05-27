#include "xcc_exec_test.h"

int main(void) {
    int a = 5;
    int b = -a;
    XCC_CHECK_EQ_INT_ID(1, b, -5);
    int c = -b;
    XCC_CHECK_EQ_INT_ID(2, c, 5);
    unsigned int d = 0xFFu;
    unsigned int e = ~d;
    XCC_CHECK_EQ_UINT_ID(3, e & 0xFFFFu, 0xFF00u);
    int f = 1;
    int g = !f;
    XCC_CHECK_EQ_INT_ID(4, g, 0);
    int h = 0;
    int j = !h;
    XCC_CHECK_EQ_INT_ID(5, j, 1);
    return 0;
}
