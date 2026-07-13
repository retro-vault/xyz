#include "xcc_exec_test.h"

unsigned char
mul181_add1(unsigned char value)
{
    return (unsigned char)(value * 181u + 1u);
}

unsigned char
mul255(unsigned char value)
{
    return (unsigned char)(value * 255u);
}

signed char
signed_mul37(signed char value)
{
    return (signed char)(value * 37);
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, mul181_add1(0xa5u), 0xaau);
    XCC_CHECK_EQ_UINT_ID(2, mul181_add1(0xffu), 0x4cu);
    XCC_CHECK_EQ_UINT_ID(3, mul255(0x81u), 0x7fu);
    XCC_CHECK_EQ_INT_ID(4, signed_mul37(-7), -3);
    XCC_CHECK_EQ_INT_ID(5, signed_mul37(7), 3);
    return 0;
}
