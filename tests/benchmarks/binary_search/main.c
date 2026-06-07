#include "bench.h"

int
main(void)
{
    static bench_u8 data[64];
    static bench_u8 query[32];
    bench_u8 i;
    bench_u8 lo;
    bench_u8 hi;
    bench_u8 mid;
    bench_u8 found;
    bench_u16 acc;

    data[0] = (bench_u8)(bench_seed_byte(0x01u) & 7u);
    for (i = 1; i < 64u; ++i)
        data[i] = (bench_u8)(data[i - 1] + 1u + (bench_seed_byte(i) & 3u));

    BENCH_FILL_ARRAY(query, 32u, 0x66u);
    acc = 0x5678u;

    for (i = 0; i < 32u; ++i) {
        lo = 0u;
        hi = 63u;
        found = 0u;
        while (lo <= hi) {
            mid = (bench_u8)((lo + hi) >> 1);
            if (data[mid] == query[i]) {
                found = 1u;
                acc = bench_mix16(acc, (bench_u16)(0x8000u | mid));
                break;
            }
            if (data[mid] < query[i])
                lo = (bench_u8)(mid + 1u);
            else {
                if (mid == 0u)
                    break;
                hi = (bench_u8)(mid - 1u);
            }
        }
        if (!found)
            acc = bench_mix16(acc, (bench_u16)(lo | 0x4000u));
    }

    return (int)acc;
}
