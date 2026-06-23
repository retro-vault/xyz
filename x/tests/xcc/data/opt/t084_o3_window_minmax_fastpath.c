typedef unsigned char u8;
typedef unsigned int u16;

static u8
bench_seed_byte(u8 salt)
{
    volatile u16 *seed = (volatile u16 *)0xff10u;
    u16 s = *seed;
    s ^= (u16)salt;
    s ^= (u16)(s << 3);
    s ^= (u16)(s >> 5);
    s += (u16)(0x31u + salt);
    return (u8)s;
}

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
    static u8 data[64];
    u8 base;
    u8 j;
    u8 k;
    u8 mn;
    u8 mx;
    u16 acc;

    data[0] = bench_seed_byte(0x11u);
    for (base = 1u; base < 64u; ++base)
        data[base] = (u8)(data[base - 1] + 1u + (u8)(0x4bu + base + 17u));

    acc = 0xbcdeu;
    for (base = 0u; base <= 56u; ++base) {
        mn = data[base];
        mx = data[base];
        for (j = 1u; j < 8u; ++j) {
            k = (u8)(base + j);
            if (data[k] < mn)
                mn = data[k];
            if (data[k] > mx)
                mx = data[k];
        }
        acc = bench_mix16(acc, mn);
        acc = bench_mix16(acc, mx);
        acc = bench_mix16(acc, (u16)((mx > mn) ? (mx - mn) : (mn - mx)));
    }

    return (int)acc;
}
