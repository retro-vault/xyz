/*
 * Shared helpers for bare-metal compiler benchmarks.
 *
 * The benchmark kernels are deliberately self-checking and libc-free.
 * Every benchmark returns a 16-bit checksum from main(), and the runner
 * compares that checksum across compilers while separately recording the
 * linked flat-binary size and emulator cycle count.
 */
#ifndef XYZ_BENCH_H
#define XYZ_BENCH_H

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

#define BENCH_MIX_ARRAY(acc, buf, n)                                    \
    do {                                                                \
        bench_u8 bench_i_;                                              \
        for (bench_i_ = 0u; bench_i_ < (bench_u8)(n); ++bench_i_)       \
            (acc) = bench_mix16((acc),                                  \
                (bench_u16)((buf)[bench_i_]) |                          \
                (bench_u16)(bench_i_ << 8));                            \
    } while (0)

static void
bench_swap_u8(bench_u8 *a, bench_u8 *b)
{
    bench_u8 t;

    t = *a;
    *a = *b;
    *b = t;
}

static bench_u16
bench_absdiff_u8(bench_u8 a, bench_u8 b)
{
    if (a > b)
        return (bench_u16)(a - b);
    return (bench_u16)(b - a);
}

#endif
