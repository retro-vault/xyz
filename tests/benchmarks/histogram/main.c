#include "bench.h"

int
main(void)
{
    static bench_u8 input[128];
    static bench_u8 bins[16];
    bench_u8 i;
    bench_u16 acc;

    BENCH_FILL_ARRAY(input, 128u, 0x99u);
    for (i = 0; i < 16u; ++i)
        bins[i] = 0u;

    for (i = 0; i < 128u; ++i)
        ++bins[(bench_u8)(input[i] >> 4)];

    acc = 0x6789u;
    for (i = 0; i < 16u; ++i)
        acc = bench_mix16(acc, (bench_u16)bins[i] | (bench_u16)(i << 8));
    return (int)acc;
}
