#include "xcc_exec_test.h"

static unsigned int
qmul(unsigned int left, unsigned int right)
{
    unsigned long product = (unsigned long)left * (unsigned long)right;
    return (unsigned int)((product >> 8) & 0xfffful);
}

int
main(void)
{
    volatile unsigned int left = 0x1234u;
    volatile unsigned int right = 0xabcdu;
    volatile unsigned long shifted = 0x89abcdeful >> 8;

    XCC_CHECK_EQ_UINT_ID(1, qmul(left, right), 0x374fu);
    XCC_CHECK_EQ_UINT_ID(2, qmul(0xffffu, 0xffffu), 0xfe00u);
    XCC_CHECK_EQ_U32_ID(3, shifted, 0xabcdu, 0x0089u);
    return 0;
}
