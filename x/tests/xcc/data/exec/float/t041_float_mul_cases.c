#include "xcc_exec_test.h"

extern unsigned long __fsmul(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, __fsmul(0xc0000000ul, 0x40600000ul), 0x0000u, 0xc0e0u);
    XCC_CHECK_EQ_U32_ID(2, __fsmul(0x3f000000ul, 0x3f000000ul), 0x0000u, 0x3e80u);
    XCC_CHECK_EQ_U32_ID(3, __fsmul(0xbfa00000ul, 0xc0800000ul), 0x0000u, 0x40a0u);
    return 0;
}
