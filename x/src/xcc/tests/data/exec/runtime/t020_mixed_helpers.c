#include "xcc_exec_test.h"

extern unsigned long __mul32(unsigned long a, unsigned long b);
extern unsigned long __fsadd(unsigned long a, unsigned long b);

int main(void) {
    unsigned int q;

    q = 42000u / 7u;
    XCC_CHECK_EQ_UINT_ID(1, q, 6000u);
    XCC_CHECK_EQ_U32_ID(2, __mul32(70000ul, 3ul), 0x3450u, 0x0003u);
    XCC_CHECK_EQ_U32_ID(3, __fsadd(0x3f800000ul, 0x40400000ul), 0x0000u, 0x4080u);
    return 0;
}
