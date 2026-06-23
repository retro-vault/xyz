#include "bench.h"

int
main(void)
{
    static bench_u8 input[96];
    static bench_u8 output[192];
    bench_u8 i;
    bench_u8 out_len;
    bench_u8 value;
    bench_u8 run;
    bench_u16 acc;

    BENCH_FILL_ARRAY(input, 96u, 0xa1u);
    for (i = 0; i < 96u; ++i) {
        if ((i & 3u) != 0u)
            input[i] = input[(bench_u8)(i - 1u)];
        else
            input[i] = (bench_u8)(input[i] & 31u);
    }

    out_len = 0u;
    i = 0u;
    while (i < 96u) {
        value = input[i];
        run = 1u;
        while ((bench_u8)(i + run) < 96u &&
               input[(bench_u8)(i + run)] == value &&
               run < 15u)
            ++run;
        output[out_len++] = run;
        output[out_len++] = value;
        i = (bench_u8)(i + run);
    }

    acc = (bench_u16)(0x1357u ^ out_len);
    BENCH_MIX_ARRAY(acc, output, out_len);
    acc = bench_mix16(acc, out_len);
    return (int)acc;
}
