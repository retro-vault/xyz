#include "bench.h"

int
main(void)
{
    static bench_u8 source[96];
    static bench_u8 gray[96];
    static bench_u8 plain[96];
    bench_u8 i;
    bench_u8 bit;

    BENCH_FILL_ARRAY(source, 96u, 0x3au);
    for (i = 0u; i < 96u; ++i) {
        gray[i] = (bench_u8)(source[i] ^ (bench_u8)(source[i] >> 1));
        plain[i] = gray[i];
        for (bit = 1u; bit < 8u; bit <<= 1)
            plain[i] ^= (bench_u8)(plain[i] >> bit);
    }

    {
        bench_u16 acc;

        acc = (bench_u16)(0x1357u ^ 96u);
        BENCH_MIX_ARRAY(acc, plain, 96u);
        return (int)acc;
    }
}
