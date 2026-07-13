#include "xcc_exec_test.h"

static unsigned int
adjust_word(unsigned int value, unsigned char predicate)
{
    value = value - (predicate == 0u);
    value = value + (predicate == 7u);
    return value;
}

static unsigned char
adjust_byte(unsigned char value, unsigned char predicate)
{
    value = (unsigned char)(value - (predicate != 0u));
    value = (unsigned char)(value + (predicate < 3u));
    return value;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, adjust_word(100u, 0u), 99u);
    XCC_CHECK_EQ_UINT_ID(2, adjust_word(100u, 7u), 101u);
    XCC_CHECK_EQ_UINT_ID(3, adjust_word(100u, 3u), 100u);
    XCC_CHECK_EQ_UINT_ID(4, adjust_byte(10u, 0u), 11u);
    XCC_CHECK_EQ_UINT_ID(5, adjust_byte(10u, 2u), 10u);
    XCC_CHECK_EQ_UINT_ID(6, adjust_byte(10u, 7u), 9u);
    return 0;
}
