#include "bench.h"

int
main(void)
{
    static bench_u8 data[64];
    bench_u8 base;
    bench_u8 j;
    bench_u8 k;
    bench_u8 mn;
    bench_u8 mx;
    bench_u16 acc;

    BENCH_FILL_ARRAY(data, 64u, 0x4bu);
    acc = 0xbcdeu;

    for (base = 0u; base <= 56u; ++base) {
        mn = data[base];
        mx = data[base];
        for (j = 1u; j < 8u; ++j) {
            k = (bench_u8)(base + j);
            if (data[k] < mn)
                mn = data[k];
            if (data[k] > mx)
                mx = data[k];
        }
        acc = bench_mix16(acc, mn);
        acc = bench_mix16(acc, mx);
        acc = bench_mix16(acc, bench_absdiff_u8(mx, mn));
    }

    return (int)acc;
}
