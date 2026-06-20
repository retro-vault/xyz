extern int fixed8_8_from_int(int x);
extern int fixed8_8_div(int a, int b);

extern long fixed16_16_from_int(int x);
extern long fixed16_16_div(long a, long b);

extern long fixed24_8_from_int(int x);
extern long fixed24_8_div(long a, long b);

int f8(int x) {
    return fixed8_8_div(x, fixed8_8_from_int(2));
}

long f16(long x) {
    return fixed16_16_div(x, fixed16_16_from_int(4));
}

long f24(long x) {
    return fixed24_8_div(x, fixed24_8_from_int(8));
}
