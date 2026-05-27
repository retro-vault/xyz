#include "xcc_exec_test.h"

extern unsigned long __mul32(unsigned long a, unsigned long b);

int main(void) {
    unsigned long a = __mul32(256ul, 256ul);
    unsigned long b = a + 0x1234ul;
    unsigned long c = b - 0x0034ul;

    XCC_CHECK_EQ_U32_ID(1, a, 0x0000u, 0x0001u);
    XCC_CHECK_EQ_U32_ID(2, b, 0x1234u, 0x0001u);
    XCC_CHECK_EQ_U32_ID(3, c, 0x1200u, 0x0001u);
    return 0;
}
