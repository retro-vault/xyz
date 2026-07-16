#include "xcc_exec_test.h"

static __attribute__((noinline)) unsigned long
sum_words(const unsigned long *words)
{
    unsigned long sum = 0;

    sum += words[0];
    sum += words[1];
    sum += words[2];
    sum += words[3];
    sum += words[4];
    sum += words[5];
    sum += words[6];
    sum += words[7];
    return sum;
}

int
main(void)
{
    static const unsigned long words[] = {
        0x01020304UL, 0x11121314UL, 0x21222324UL, 0x31323334UL,
        0x41424344UL, 0x51525354UL, 0x61626364UL, 0x71727374UL
    };

    XCC_CHECK_EQ_ULONG_ID(1, sum_words(words), 0xc9d1d9e0UL);
    return 0;
}
