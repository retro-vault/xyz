#include "xcc_exec_test.h"

int main(void) {
    unsigned int x = 0x55aau;
    unsigned int y = 0x0f0fu;

    XCC_CHECK_EQ_UINT_ID(1, x & y, 0x050au);
    XCC_CHECK_EQ_UINT_ID(2, x | y, 0x5fafu);
    XCC_CHECK_EQ_UINT_ID(3, x ^ y, 0x5aa5u);
    XCC_CHECK_EQ_UINT_ID(4, (unsigned int)~0x1234u, 0xedcbu);
    return 0;
}
