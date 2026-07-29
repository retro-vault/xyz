#include "xcc_exec_test.h"

static __attribute__((noinline)) unsigned long
fused_indexed_step(const unsigned long *seed, const unsigned long *input)
{
    unsigned long a = seed[0];
    unsigned long b = input[0];
    unsigned long c = input[1];
    unsigned long d = input[2];

    return (d ^ (b & (c ^ d))) + input[3] + 0xd76aa478ul + a;
}

int
main(void)
{
    static const unsigned long seed[] = {
        0x12345678ul
    };
    static const unsigned long input[] = {
        0x89abcdeful, 0x0f1e2d3cul, 0x76543210ul, 0xf0f00f0ful
    };

    XCC_CHECK_EQ_U32_ID(1, fused_indexed_step(seed, input),
                        0x493bu, 0x59edu);
    XCC_CHECK_EQ_U32_ID(2, fused_indexed_step(seed, input),
                        0x493bu, 0x59edu);
    return 0;
}
