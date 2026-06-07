#include "bench.h"

int
main(void)
{
    static bench_u8 code[96];
    static bench_u8 mem[16];
    bench_u8 pc;
    bench_u8 acc;
    bench_u8 x;
    bench_u8 y;
    bench_u8 op;
    bench_u16 mix;

    BENCH_FILL_ARRAY(code, 96u, 0x5cu);
    BENCH_FILL_ARRAY(mem, 16u, 0x6du);

    pc = 0u;
    acc = bench_seed_byte(0x1u);
    x = bench_seed_byte(0x2u);
    y = bench_seed_byte(0x3u);
    mix = 0xcdefu;

    while (pc < 96u) {
        op = (bench_u8)(code[pc] & 7u);
        switch (op) {
        case 0:
            acc = (bench_u8)(acc + mem[code[pc] & 15u]);
            break;
        case 1:
            acc ^= mem[(bench_u8)((code[pc] >> 1) & 15u)];
            break;
        case 2:
            x = (bench_u8)(x + acc + 1u);
            break;
        case 3:
            y = (bench_u8)(y ^ (bench_u8)(acc + x));
            break;
        case 4:
            mem[(bench_u8)(pc & 15u)] = (bench_u8)(mem[(bench_u8)(pc & 15u)] + y);
            break;
        case 5:
            if ((acc & 1u) != 0u && pc < 94u)
                ++pc;
            break;
        case 6:
            acc = (bench_u8)((acc << 1) | (acc >> 7));
            break;
        default:
            acc = (bench_u8)(acc + x + y);
            break;
        }
        mix = bench_mix16(mix, (bench_u16)(acc | (bench_u16)(x << 8)));
        mix = bench_mix16(mix, y);
        ++pc;
    }

    return (int)mix;
}
