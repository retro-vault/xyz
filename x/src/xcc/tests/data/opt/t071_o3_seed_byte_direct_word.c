typedef unsigned char bench_u8;
typedef unsigned int bench_u16;

unsigned char f(unsigned char salt) {
    volatile bench_u16 *seedp = (volatile bench_u16 *)0xff10u;
    bench_u16 s;

    s = *seedp;
    s ^= (bench_u16)salt;
    s ^= (bench_u16)(s << 3);
    s ^= (bench_u16)(s >> 5);
    s += (bench_u16)(0x31u + salt);
    return (unsigned char)s;
}
