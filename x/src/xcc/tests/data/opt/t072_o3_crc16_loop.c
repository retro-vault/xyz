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

static bench_u8 buf[96];

bench_u16 f(void)
{
    bench_u16 crc;
    bench_u16 i;
    bench_u8 bit;

    BENCH_FILL_ARRAY(buf, 96u, 0x11u);

    crc = (bench_u16)(0x1021u ^ bench_seed_word());
    for (i = 0; i < 96u; ++i) {
        crc ^= (bench_u16)buf[i] << 8;
        for (bit = 0; bit < 8u; ++bit) {
            if (crc & 0x8000u)
                crc = (bench_u16)((crc << 1) ^ 0x1021u);
            else
                crc = (bench_u16)(crc << 1);
        }
        crc = bench_mix16(crc, buf[i]);
    }

    return crc;
}
