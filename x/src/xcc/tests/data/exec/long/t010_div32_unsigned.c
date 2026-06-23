#include "xcc_exec_test.h"

extern unsigned long __div32(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, __div32(4000000000ul, 1000ul), 0x0900u, 0x003du);
    XCC_CHECK_EQ_U32_ID(2, __div32(3000000001ul, 7ul), 0x7b24u, 0x198bu);
    XCC_CHECK_EQ_U32_ID(3, __div32(65536ul, 256ul), 0x0100u, 0x0000u);
    return 0;
}
