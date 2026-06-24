#include "xcc_exec_test.h"

extern unsigned long __fsmul(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, __fsmul(0x3fc00000ul, 0x40100000ul), 0x0000u, 0x4058u);
    XCC_CHECK_EQ_U32_ID(2, __fsmul(0x40000000ul, 0x3f000000ul), 0x0000u, 0x3f80u);
    XCC_CHECK_EQ_U32_ID(3, __fsmul(0x00000000ul, 0x40400000ul), 0x0000u, 0x0000u);
    return 0;
}
