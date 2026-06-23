extern int fixed8_8_from_int(int x);
extern int fixed8_8_div(int a, int b);
extern int fixed8_8_mul(int a, int b);

extern long fixed16_16_from_int(int x);
extern long fixed16_16_div(long a, long b);
extern long fixed16_16_div4(long a);
extern long fixed16_16_mul(long a, long b);
extern long fixed16_16_mul1_2(long a);

extern long fixed24_8_from_int(int x);
extern long fixed24_8_div(long a, long b);
extern long fixed24_8_mul(long a, long b);
extern long fixed24_8_mul5_4(long a);

int f8(void) {
    return fixed8_8_div(fixed8_8_from_int(1), fixed8_8_from_int(8));
}

long f16(void) {
    return fixed16_16_div(fixed16_16_from_int(3), fixed16_16_from_int(2));
}

long f24(void) {
    return fixed24_8_div(fixed24_8_from_int(3), fixed24_8_from_int(2));
}

int f8_negative_half_raw(void) {
    return fixed8_8_mul(-1, fixed8_8_div(fixed8_8_from_int(1),
                                         fixed8_8_from_int(2)));
}

long f16_negative_half_raw(void) {
    return fixed16_16_mul(-1L, fixed16_16_div(fixed16_16_from_int(1),
                                              fixed16_16_from_int(2)));
}

long f24_negative_half_raw(void) {
    return fixed24_8_mul(-1L, fixed24_8_div(fixed24_8_from_int(1),
                                            fixed24_8_from_int(2)));
}

long f16_div4_specialized(void) {
    return fixed16_16_div4(fixed16_16_from_int(5));
}

long f16_mul1_2_specialized_negative_raw(void) {
    return fixed16_16_mul1_2(-1L);
}

long f24_mul5_4_specialized(void) {
    return fixed24_8_mul5_4(fixed24_8_from_int(4));
}
