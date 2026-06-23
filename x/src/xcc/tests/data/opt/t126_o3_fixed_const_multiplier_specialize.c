extern int fixed8_8_from_int(int x);
extern int fixed8_8_div(int a, int b);
extern int fixed8_8_mul(int a, int b);

extern long fixed16_16_from_int(int x);
extern long fixed16_16_div(long a, long b);
extern long fixed16_16_mul(long a, long b);

extern long fixed24_8_from_int(int x);
extern long fixed24_8_div(long a, long b);
extern long fixed24_8_mul(long a, long b);

int f8(int x) {
    return fixed8_8_mul(x, fixed8_8_div(fixed8_8_from_int(1),
                                        fixed8_8_from_int(2)));
}

long f16(long x) {
    return fixed16_16_mul(x, fixed16_16_div(fixed16_16_from_int(3),
                                            fixed16_16_from_int(2)));
}

long f24(long x) {
    return fixed24_8_mul(fixed24_8_div(fixed24_8_from_int(5),
                                       fixed24_8_from_int(4)),
                         x);
}

long f16_half(long x) {
    return fixed16_16_mul(x, fixed16_16_div(fixed16_16_from_int(1),
                                            fixed16_16_from_int(2)));
}

long f24_quarter(long x) {
    return fixed24_8_mul(x, fixed24_8_div(fixed24_8_from_int(1),
                                          fixed24_8_from_int(4)));
}
