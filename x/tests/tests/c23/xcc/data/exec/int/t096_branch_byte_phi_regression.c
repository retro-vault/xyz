#include "xcc_exec_test.h"

static volatile unsigned char g_bytes[12] = {
    0x13u, 0x80u, 0x2du, 0x04u, 0xffu, 0x31u,
    0x55u, 0xaau, 0x01u, 0x7eu, 0xc3u, 0x18u
};

static unsigned char
branch_arith(unsigned char value)
{
    unsigned char i;

    for (i = 0u; i < 12u; ++i) {
        unsigned char rhs = g_bytes[i];
        if ((value & 0x80u) != 0u)
            value = (unsigned char)((value << 1) ^ rhs);
        else
            value = (unsigned char)((value << 1) + rhs);

        if ((rhs & 1u) != 0u)
            value = (unsigned char)(value - (unsigned char)(i * 3u + 1u));
        else
            value = (unsigned char)(value | (unsigned char)(1u << (i & 7u)));
    }
    return value;
}

static unsigned char
branch_unary(unsigned char value)
{
    unsigned char i;

    for (i = 0u; i < 12u; ++i) {
        unsigned char rhs = g_bytes[i];
        if ((rhs & 1u) != 0u)
            value = (unsigned char)~value;
        else
            value = (unsigned char)(0u - value);

        if ((rhs & 4u) != 0u)
            value = (unsigned char)(value * 5u);
        else
            value = (unsigned char)(value ^ rhs);
    }
    return value;
}

static unsigned char
branch_mask(unsigned char value)
{
    unsigned char i;

    for (i = 0u; i < 12u; ++i) {
        unsigned char rhs = g_bytes[i];
        if ((rhs & 0x80u) != 0u)
            value = (unsigned char)(value & rhs);
        else
            value = (unsigned char)(value | rhs);

        if ((value & 1u) != 0u)
            value = (unsigned char)(value + 17u);
        else
            value = (unsigned char)(value - 9u);
    }
    return value;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, branch_arith(0x5au), 184u);
    XCC_CHECK_EQ_UINT_ID(2, branch_unary(0x00u), 53u);
    XCC_CHECK_EQ_UINT_ID(3, branch_unary(0x5au), 19u);
    XCC_CHECK_EQ_UINT_ID(4, branch_unary(0xc7u), 172u);
    XCC_CHECK_EQ_UINT_ID(5, branch_mask(0x5au), 81u);
    XCC_CHECK_EQ_UINT_ID(6, branch_mask(0xc7u), 209u);
    return 0;
}
