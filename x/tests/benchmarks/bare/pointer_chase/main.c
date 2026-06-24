#include "bench.h"

int
main(void)
{
    static bench_u8 next[64];
    static bench_u8 values[64];
    bench_u8 pos;
    bench_u8 step;
    bench_u8 i;
    bench_u16 acc;

    BENCH_FILL_ARRAY(values, 64u, 0x55u);

    pos = bench_seed_byte(0x12u) & 63u;
    for (i = 0; i < 64u; ++i) {
        pos = (bench_u8)((pos + 5u) & 63u);
        next[i] = pos;
    }

    pos = bench_seed_byte(0x34u) & 63u;
    acc = 0x4567u;
    for (step = 0; step < 0xffu; ++step) {
        pos = next[pos];
        acc = bench_mix16(acc, values[pos]);
        acc = bench_mix16(acc, pos);
    }

    return (int)acc;
}
