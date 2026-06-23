typedef unsigned char bench_u8;
typedef unsigned int  bench_u16;

static bench_u16
bench_seed_word(void)
{
    return *((volatile bench_u16 *)0xff10u);
}

static bench_u8
bench_seed_byte(bench_u8 salt)
{
    bench_u16 s;

    s = bench_seed_word();
    s ^= (bench_u16)salt;
    s ^= (bench_u16)(s << 3);
    s ^= (bench_u16)(s >> 5);
    s += (bench_u16)(0x31u + salt);
    return (bench_u8)s;
}

#define BENCH_FILL_ARRAY(buf, n, salt)                                  \
    do {                                                                \
        bench_u8 bench_i_;                                              \
        bench_u8 bench_v_;                                              \
        bench_v_ = bench_seed_byte((bench_u8)((salt) ^ 0x5au));         \
        for (bench_i_ = 0u; bench_i_ < (bench_u8)(n); ++bench_i_) {     \
            bench_v_ ^= (bench_u8)(bench_v_ << 3);                      \
            bench_v_ ^= (bench_u8)(bench_v_ >> 5);                      \
            bench_v_ += (bench_u8)((salt) + bench_i_ + 17u);            \
            (buf)[bench_i_] = (bench_u8)(bench_v_ ^ bench_i_);          \
        }                                                               \
    } while (0)

static bench_u16
bench_mix16(bench_u16 acc, bench_u16 value)
{
    acc ^= (bench_u16)(value + 0x9e37u);
    acc = (bench_u16)((acc << 5) | (acc >> 11));
    acc += (bench_u16)(value ^ 0x7f4au);
    return acc;
}

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
