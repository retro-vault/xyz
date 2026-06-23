#include "xcc_exec_test.h"

int main(void) {
    unsigned int u = 0x8000u;
    int s = -2;

    XCC_CHECK_ID(1, 1 < 2);
    XCC_CHECK_ID(2, 400 > 399);
    XCC_CHECK_ID(3, (u >> 1) == 0x4000u);
    XCC_CHECK_EQ_UINT_ID(4, 1u << 8, 256u);
    XCC_CHECK_EQ_INT_ID(5, s >> 1, -1);
    return 0;
}
