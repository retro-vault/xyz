typedef unsigned char u8;
typedef unsigned int u16;

u16 f(void) {
    static u8 input[128];
    static u8 bins[16];
    u8 i;
    u16 acc;

    for (i = 0; i < 16u; ++i)
        bins[i] = 0u;

    for (i = 0; i < 128u; ++i)
        ++bins[(u8)(input[i] >> 4)];

    acc = 0x6789u;
    for (i = 0; i < 16u; ++i)
        acc = (u16)(acc + ((u16)bins[i] | (u16)(i << 8)));
    return acc;
}
