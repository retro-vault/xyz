#include "xcc_exec_test.h"

extern unsigned long __fsdiv(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, __fsdiv(0x40e00000ul, 0x40000000ul), 0x0000u, 0x4060u);
    XCC_CHECK_EQ_U32_ID(2, __fsdiv(0xc1100000ul, 0x40400000ul), 0x0000u, 0xc040u);
    XCC_CHECK_EQ_U32_ID(3, __fsdiv(0x3f800000ul, 0x40800000ul), 0x0000u, 0x3e80u);
    return 0;
}
