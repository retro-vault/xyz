#include "bench.h"

int
main(void)
{
    static bench_u8 prime[128];
    bench_u8 p;
    bench_u8 j;
    bench_u16 acc;

    for (p = 0u; p < 128u; ++p)
        prime[p] = 1u;
    prime[0] = 0u;
    prime[1] = 0u;

    for (p = 2u; p < 128u; ++p) {
        if (prime[p] == 0u)
            continue;
        j = (bench_u8)(p + p);
        while (j < 128u) {
            prime[j] = 0u;
            j = (bench_u8)(j + p);
        }
    }

    acc = (bench_u16)(0x1357u ^ 128u);
    BENCH_MIX_ARRAY(acc, prime, 128u);
    for (p = 0u; p < 128u; ++p) {
        if (prime[p] != 0u)
            acc = bench_mix16(acc, p);
    }
    return (int)acc;
}
