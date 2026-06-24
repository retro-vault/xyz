#include "xcc_exec_test.h"

extern unsigned long __fsadd(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, __fsadd(0x3f800000ul, 0x3f800000ul), 0x0000u, 0x4000u);
    XCC_CHECK_EQ_U32_ID(2, __fsadd(0x40200000ul, 0xbf000000ul), 0x0000u, 0x4000u);
    XCC_CHECK_EQ_U32_ID(3, __fsadd(0x40700000ul, 0x3e800000ul), 0x0000u, 0x4080u);
    return 0;
}
