typedef unsigned char bench_u8;
typedef unsigned int bench_u16;

bench_u16 bench_seed_word(void);

unsigned char f(unsigned char salt) {
    bench_u16 s;

    s = bench_seed_word();
    s ^= (bench_u16)salt;
    s ^= (bench_u16)(s << 3);
    s ^= (bench_u16)(s >> 5);
    s += (bench_u16)(0x31u + salt);
    return (unsigned char)s;
}
