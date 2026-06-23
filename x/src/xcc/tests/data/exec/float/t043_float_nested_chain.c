#include "xcc_exec_test.h"

extern unsigned long __fsadd(unsigned long a, unsigned long b);
extern unsigned long __fssub(unsigned long a, unsigned long b);
extern unsigned long __fsmul(unsigned long a, unsigned long b);
extern unsigned long __fsdiv(unsigned long a, unsigned long b);

int main(void) {
    unsigned long t = __fsadd(0x3f800000ul, 0x40000000ul);
    unsigned long u = __fssub(t, 0x3f000000ul);
    unsigned long v = __fsmul(u, 0x40000000ul);

    XCC_CHECK_EQ_U32_ID(1, t, 0x0000u, 0x4040u);
    XCC_CHECK_EQ_U32_ID(2, u, 0x0000u, 0x4020u);
    XCC_CHECK_EQ_U32_ID(3, v, 0x0000u, 0x40a0u);
    return 0;
}
