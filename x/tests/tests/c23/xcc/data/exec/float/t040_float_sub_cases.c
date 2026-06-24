#include "xcc_exec_test.h"

extern unsigned long __fssub(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, __fssub(0x40b00000ul, 0x40100000ul), 0x0000u, 0x4050u);
    XCC_CHECK_EQ_U32_ID(2, __fssub(0x3f800000ul, 0x40000000ul), 0x0000u, 0xbf80u);
    XCC_CHECK_EQ_U32_ID(3, __fssub(0xc0400000ul, 0xbfc00000ul), 0x0000u, 0xbfc0u);
    return 0;
}
