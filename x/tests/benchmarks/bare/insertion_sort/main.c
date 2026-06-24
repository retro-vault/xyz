#include "bench.h"

int
main(void)
{
    static bench_u8 data[48];
    bench_u8 i;
    bench_u8 j;
    bench_u8 key;

    BENCH_FILL_ARRAY(data, 48u, 0x77u);

    for (i = 1u; i < 48u; ++i) {
        key = data[i];
        j = i;
        while (j > 0u && data[(bench_u8)(j - 1u)] > key) {
            data[j] = data[(bench_u8)(j - 1u)];
            --j;
        }
        data[j] = key;
    }

    {
        bench_u16 acc;

        acc = (bench_u16)(0x1357u ^ 48u);
        BENCH_MIX_ARRAY(acc, data, 48u);
        return (int)acc;
    }
}
