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

static u8 data[128];

int
main(void)
{
    u8 i;
    u16 acc;

    for (i = 0u; i < 128u; ++i)
        data[i] = (u8)(i & 1u);

    acc = (u16)(0x1357u ^ 128u);
    for (i = 0u; i < 128u; ++i) {
        if (data[i] != 0u)
            acc = bench_mix16(acc, i);
    }

    return (int)acc;
}
