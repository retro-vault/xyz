#include "xcc_exec_test.h"

extern unsigned long __fsmul(unsigned long a, unsigned long b);
extern unsigned long __fsdiv(unsigned long a, unsigned long b);

int main(void) {
    unsigned long a = __fsmul(0x40400000ul, 0xc0000000ul);
    unsigned long b = __fsdiv(0x40a00000ul, 0x40000000ul);
    unsigned long c = __fsmul(0x3f000000ul, 0x40800000ul);

    XCC_CHECK_EQ_U32_ID(1, a, 0x0000u, 0xc0c0u);
    XCC_CHECK_EQ_U32_ID(2, b, 0x0000u, 0x4020u);
    XCC_CHECK_EQ_U32_ID(3, c, 0x0000u, 0x4000u);
    return 0;
}
