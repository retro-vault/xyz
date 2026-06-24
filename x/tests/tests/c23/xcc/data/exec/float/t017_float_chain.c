#include "xcc_exec_test.h"

extern unsigned long __fsadd(unsigned long a, unsigned long b);
extern unsigned long __fssub(unsigned long a, unsigned long b);
extern unsigned long __fsmul(unsigned long a, unsigned long b);

int main(void) {
    unsigned long t = __fsmul(0x40000000ul, 0x40400000ul);
    unsigned long r = __fsadd(0x3f800000ul, t);
    unsigned long s = __fssub(r, 0x40400000ul);

    XCC_CHECK_EQ_U32_ID(1, t, 0x0000u, 0x40c0u);
    XCC_CHECK_EQ_U32_ID(2, r, 0x0000u, 0x40e0u);
    XCC_CHECK_EQ_U32_ID(3, s, 0x0000u, 0x4080u);
    return 0;
}
