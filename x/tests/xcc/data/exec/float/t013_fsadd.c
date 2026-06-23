#include "xcc_exec_test.h"

extern unsigned long __fsadd(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, __fsadd(0x3fc00000ul, 0x40100000ul), 0x0000u, 0x4070u);
    XCC_CHECK_EQ_U32_ID(2, __fsadd(0x3f000000ul, 0x3f000000ul), 0x0000u, 0x3f80u);
    XCC_CHECK_EQ_U32_ID(3, __fsadd(0x00000000ul, 0x3f800000ul), 0x0000u, 0x3f80u);
    return 0;
}
