#include "bench.h"

int
main(void)
{
    static bench_u8 a[64];
    static bench_u8 b[64];
    bench_u8 r;
    bench_u8 c;
    bench_u8 gen;
    bench_u16 acc;

    BENCH_FILL_ARRAY(a, 64u, 0xf6u);
    for (r = 0; r < 64u; ++r)
        a[r] &= 1u;

    for (gen = 0u; gen < 6u; ++gen) {
        for (r = 0u; r < 8u; ++r) {
            for (c = 0u; c < 8u; ++c) {
                bench_u8 count;
                bench_u8 rr0;
                bench_u8 rr1;
                bench_u8 rr2;
                bench_u8 cc0;
                bench_u8 cc1;
                bench_u8 cc2;
                bench_u8 idx;

                rr0 = (r == 0u) ? 7u : (bench_u8)(r - 1u);
                rr1 = r;
                rr2 = (r == 7u) ? 0u : (bench_u8)(r + 1u);
                cc0 = (c == 0u) ? 7u : (bench_u8)(c - 1u);
                cc1 = c;
                cc2 = (c == 7u) ? 0u : (bench_u8)(c + 1u);

                count = 0u;
                count += a[(bench_u8)(rr0 * 8u + cc0)];
                count += a[(bench_u8)(rr0 * 8u + cc1)];
                count += a[(bench_u8)(rr0 * 8u + cc2)];
                count += a[(bench_u8)(rr1 * 8u + cc0)];
                count += a[(bench_u8)(rr1 * 8u + cc2)];
                count += a[(bench_u8)(rr2 * 8u + cc0)];
                count += a[(bench_u8)(rr2 * 8u + cc1)];
                count += a[(bench_u8)(rr2 * 8u + cc2)];

                idx = (bench_u8)(r * 8u + c);
                if (a[idx] != 0u)
                    b[idx] = (count == 2u || count == 3u) ? 1u : 0u;
                else
                    b[idx] = (count == 3u) ? 1u : 0u;
            }
        }

        for (r = 0u; r < 64u; ++r)
            a[r] = b[r];
    }

    acc = (bench_u16)(0x1357u ^ 64u);
    BENCH_MIX_ARRAY(acc, a, 64u);
    return (int)acc;
}
