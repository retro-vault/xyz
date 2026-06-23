#include "bench.h"

int
main(void)
{
    static bench_u8 src[64];
    static bench_u8 dst[64];
    static bench_u8 row_sum[8];
    static bench_u8 col_sum[8];
    bench_u8 r;
    bench_u8 c;
    bench_u16 acc;
    bench_u8 idx;

    BENCH_FILL_ARRAY(src, 64u, 0x44u);

    for (r = 0; r < 8u; ++r) {
        row_sum[r] = 0u;
        col_sum[r] = 0u;
    }

    for (r = 0; r < 8u; ++r) {
        for (c = 0; c < 8u; ++c) {
            idx = (bench_u8)(r * 8u + c);
            row_sum[r] = (bench_u8)(row_sum[r] + src[idx]);
            col_sum[c] = (bench_u8)(col_sum[c] + src[idx]);
        }
    }

    for (r = 0; r < 8u; ++r) {
        for (c = 0; c < 8u; ++c) {
            bench_u8 a;
            bench_u8 b;

            a = src[(bench_u8)(r * 8u + c)];
            b = src[(bench_u8)(c * 8u + r)];
            dst[(bench_u8)(r * 8u + c)] =
                (bench_u8)(a + (b >> 1) + (row_sum[r] ^ col_sum[c]));
        }
    }

    acc = (bench_u16)(0x1357u ^ 64u);
    BENCH_MIX_ARRAY(acc, dst, 64u);
    for (r = 0; r < 8u; ++r) {
        acc = bench_mix16(acc, row_sum[r]);
        acc = bench_mix16(acc, col_sum[r]);
        acc = bench_mix16(acc, dst[(bench_u8)(r * 8u + r)]);
    }

    return (int)acc;
}
