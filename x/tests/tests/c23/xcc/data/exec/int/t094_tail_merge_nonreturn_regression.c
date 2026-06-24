#include "xcc_exec_test.h"

static volatile unsigned short g_seed0 = 0x1d3bu;
static volatile unsigned short g_seed1 = 0xa592u;

static unsigned short
mix16(unsigned short acc, unsigned short value)
{
    acc ^= (unsigned short)(value + 0x1357u);
    acc = (unsigned short)((acc << 5) | (acc >> 11));
    acc += (unsigned short)(value ^ 0x2468u);
    return acc;
}

static unsigned short
tail_merge_kernel(unsigned char limit, unsigned short seed)
{
    unsigned char i;
    unsigned short acc;

    acc = (unsigned short)(seed ^ 0x55aau);
    for (i = 0u; i < limit; ++i) {
        unsigned short lhs;
        unsigned short rhs;
        unsigned short step;

        step = (unsigned short)((unsigned short)i * 17u);
        if ((i & 1u) != 0u) {
            lhs = (unsigned short)(acc + step + 3u);
            rhs = (unsigned short)(seed ^ (unsigned short)(i << 8));
            lhs = (unsigned short)(((unsigned short)(lhs << 1)) |
                                   ((unsigned short)(lhs >> 15)));
            lhs = (unsigned short)(lhs ^ rhs);
            rhs = (unsigned short)(rhs + 0x0033u + step);
        } else {
            lhs = (unsigned short)(acc ^ (unsigned short)(0x1100u + i));
            rhs = (unsigned short)(seed + (unsigned short)(i * 9u));
            lhs = (unsigned short)(((unsigned short)(lhs << 1)) |
                                   ((unsigned short)(lhs >> 15)));
            lhs = (unsigned short)(lhs ^ rhs);
            rhs = (unsigned short)(rhs + 0x0033u + step);
        }

        acc = mix16(lhs, rhs);

        if ((acc & 3u) == 1u)
            acc = (unsigned short)(acc ^ (unsigned short)(0x5500u + i));
        else if ((acc & 7u) == 6u)
            acc = (unsigned short)(acc + rhs);
    }

    return acc;
}

int
main(void)
{
    unsigned short a;
    unsigned short b;
    unsigned short c;

    a = tail_merge_kernel(19u, g_seed0);
    b = tail_merge_kernel(23u, g_seed1);
    c = tail_merge_kernel(9u, (unsigned short)(g_seed0 ^ g_seed1));

    XCC_CHECK_EQ_UINT_ID(1, a, 47575u);
    XCC_CHECK_EQ_UINT_ID(2, b, 27090u);
    XCC_CHECK_EQ_UINT_ID(3, c, 22075u);
    XCC_CHECK_EQ_UINT_ID(4, (unsigned short)(a ^ b ^ c), 34366u);
    return 0;
}
