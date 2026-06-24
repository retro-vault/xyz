#include "bench.h"

int
main(void)
{
    static bench_u8 samples[64];
    bench_u16 acc;
    bench_u16 mix;
    bench_u8 i;

    BENCH_FILL_ARRAY(samples, 64u, 0xb2u);
    mix = 0x789au;

    for (i = 7u; i < 64u; ++i) {
        acc = (bench_u16)samples[i];
        acc += (bench_u16)(samples[(bench_u8)(i - 1u)] << 1);
        acc += (bench_u16)samples[(bench_u8)(i - 2u)];
        acc -= (bench_u16)samples[(bench_u8)(i - 3u)];
        acc += (bench_u16)(samples[(bench_u8)(i - 5u)] << 1);
        acc += (bench_u16)samples[(bench_u8)(i - 5u)];
        acc -= (bench_u16)(samples[(bench_u8)(i - 6u)] << 1);
        acc += (bench_u16)samples[(bench_u8)(i - 7u)];
        mix = bench_mix16(mix, (bench_u16)(acc & 0xffu));
    }

    return (int)mix;
}
