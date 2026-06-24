#include "xcc_exec_test.h"

extern unsigned long __fssub(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, __fssub(0x40100000ul, 0x3fc00000ul), 0x0000u, 0x3f40u);
    XCC_CHECK_EQ_U32_ID(2, __fssub(0x3f800000ul, 0x3f800000ul), 0x0000u, 0x0000u);
    XCC_CHECK_EQ_U32_ID(3, __fssub(0x40800000ul, 0x40000000ul), 0x0000u, 0x4000u);
    return 0;
}
