typedef unsigned char u8;
typedef unsigned int u16;

static u16
bench_mix16(u16 acc, u16 value)
{
    acc ^= (u16)(value + 0x9e37u);
    acc = (u16)((acc << 5) | (acc >> 11));
    acc += (u16)(value ^ 0x7f4au);
    return acc;
}

int
main(void)
{
    static u8 dst[64];
    static u8 row_sum[8];
    static u8 col_sum[8];
    u8 r;
    u16 acc;

    acc = (u16)(0x1357u ^ 64u);
    for (r = 0; r < 8u; ++r) {
        acc = bench_mix16(acc, row_sum[r]);
        acc = bench_mix16(acc, col_sum[r]);
        acc = bench_mix16(acc, dst[(u8)(r * 8u + r)]);
    }

    return (int)acc;
}
