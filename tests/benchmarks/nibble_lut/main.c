#include "bench.h"

int
main(void)
{
    static const bench_u8 lut[16] = {
        0x0u, 0x3u, 0x5u, 0x9u,
        0x6u, 0xau, 0xcu, 0xfu,
        0x1u, 0x4u, 0x7u, 0x8u,
        0x2u, 0xbu, 0xdu, 0xeu
    };
    static bench_u8 input[96];
    static bench_u8 output[96];
    bench_u16 i;

    BENCH_FILL_ARRAY(input, 96u, 0xe5u);
    for (i = 0; i < 96u; ++i) {
        bench_u8 lo;
        bench_u8 hi;

        lo = lut[input[i] & 15u];
        hi = lut[(bench_u8)(input[i] >> 4)];
        output[i] = (bench_u8)(lo | (bench_u8)(hi << 4));
    }

    {
        bench_u16 acc;

        acc = (bench_u16)(0x1357u ^ 96u);
        BENCH_MIX_ARRAY(acc, output, 96u);
        return (int)acc;
    }
}
