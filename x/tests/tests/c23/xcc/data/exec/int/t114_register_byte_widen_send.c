#include "xcc_exec_test.h"

static volatile unsigned int captured_value;

unsigned int
consume_promoted_byte(unsigned int value)
{
    captured_value = value;
    return value ^ 0x55aau;
}

unsigned int
forward_promoted_byte(unsigned char value)
{
    return consume_promoted_byte(value);
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, forward_promoted_byte(0xa5u), 0x550fu);
    XCC_CHECK_EQ_UINT_ID(2, captured_value, 0x00a5u);
    XCC_CHECK_EQ_UINT_ID(3, forward_promoted_byte(0xffu), 0x5555u);
    XCC_CHECK_EQ_UINT_ID(4, captured_value, 0x00ffu);
    return 0;
}
