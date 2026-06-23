#include "xcc_exec_test.h"

extern unsigned int __div16(unsigned int a, unsigned int b);
extern unsigned int __mod16(unsigned int a, unsigned int b);
extern unsigned long __fsadd(unsigned long a, unsigned long b);

int main(void) {
    unsigned int q = __div16(3210u, 7u);
    unsigned int r = __mod16(60000u, 7u);
    unsigned long f = __fsadd(0x3f800000ul, 0x40800000ul);

    XCC_CHECK_EQ_UINT_ID(1, q, 458u);
    XCC_CHECK_EQ_UINT_ID(2, r, 3u);
    XCC_CHECK_EQ_U32_ID(3, f, 0x0000u, 0x40a0u);
    return 0;
}
