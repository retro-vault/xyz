#include "xcc_exec_test.h"

extern unsigned long __mod32(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, __mod32(4000000000ul, 1000ul), 0x0000u, 0x0000u);
    XCC_CHECK_EQ_U32_ID(2, __mod32(3000000001ul, 7ul), 0x0005u, 0x0000u);
    XCC_CHECK_EQ_U32_ID(3, __mod32(65537ul, 256ul), 0x0001u, 0x0000u);
    return 0;
}
