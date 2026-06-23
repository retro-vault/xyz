#include "xcc_exec_test.h"

int main(void) {
    unsigned long x = 0xffff0000ul;
    unsigned long y = x + 0x1234ul;
    unsigned long z = y - 0x0034ul;

    XCC_CHECK_EQ_U32_ID(1, y, 0x1234u, 0xffffu);
    XCC_CHECK_EQ_U32_ID(2, z, 0x1200u, 0xffffu);
    XCC_CHECK_EQ_U32_ID(3, z + 0x10000ul, 0x1200u, 0x0000u);
    return 0;
}
