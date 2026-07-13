#include "xcc_exec_test.h"

static unsigned short g_words[20] = {
    0x1234u, 0x00ffu, 0xabcdu, 0x0102u,
    0x8001u, 0x7fffu, 0x2222u, 0x3333u,
    0x4444u, 0x5555u, 0x6666u, 0x7777u,
    0x8888u, 0x9999u, 0xaaaau, 0xbbbbu,
    0xccccu, 0xddddu, 0xeeeeu, 0xffffu
};

static unsigned short
sum_words(const unsigned short *words)
{
    unsigned short sum = 0u;
    unsigned char i;

    for (i = 0u; i < 16u; ++i)
        sum = (unsigned short)(sum + words[i]);
    return sum;
}

static unsigned short
weighted_sum(const unsigned short *words)
{
    unsigned short sum = 0u;
    unsigned char i;

    for (i = 0u; i < 16u; ++i) {
        sum = (unsigned short)(sum +
              (unsigned short)(words[i] * (unsigned short)(i + 1u)));
    }
    return sum;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, sum_words(g_words), 5459u);
    XCC_CHECK_EQ_UINT_ID(2, weighted_sum(g_words), 61630u);
    XCC_CHECK_EQ_UINT_ID(3, sum_words(g_words + 4), 61159u);
    return 0;
}
