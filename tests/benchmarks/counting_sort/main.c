#include "bench.h"

int
main(void)
{
    static bench_u8 input[64];
    static bench_u8 output[64];
    static bench_u8 count[16];
    bench_u8 i;
    bench_u8 v;
    bench_u8 pos;
    bench_u16 acc;

    BENCH_FILL_ARRAY(input, 64u, 0x88u);
    for (i = 0; i < 16u; ++i)
        count[i] = 0u;

    for (i = 0; i < 64u; ++i) {
        v = (bench_u8)(input[i] & 15u);
        ++count[v];
    }

    pos = 0u;
    for (v = 0u; v < 16u; ++v) {
        while (count[v] != 0u) {
            output[pos++] = v;
            --count[v];
        }
    }

    acc = (bench_u16)(0x1357u ^ 64u);
    BENCH_MIX_ARRAY(acc, output, 64u);
    for (i = 1u; i < 64u; ++i)
        acc = bench_mix16(acc, bench_absdiff_u8(output[i], output[(bench_u8)(i - 1u)]));
    return (int)acc;
}
