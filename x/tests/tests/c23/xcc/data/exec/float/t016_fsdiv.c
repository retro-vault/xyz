#include "xcc_exec_test.h"

extern unsigned long __fsdiv(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, __fsdiv(0x40700000ul, 0x3fc00000ul), 0x0000u, 0x4020u);
    XCC_CHECK_EQ_U32_ID(2, __fsdiv(0x3f800000ul, 0x40000000ul), 0x0000u, 0x3f00u);
    XCC_CHECK_EQ_U32_ID(3, __fsdiv(0x40800000ul, 0x3f800000ul), 0x0000u, 0x4080u);
    return 0;
}
