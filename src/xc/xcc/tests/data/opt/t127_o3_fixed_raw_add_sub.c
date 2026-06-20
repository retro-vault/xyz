extern int fixed8_8_add(int a, int b);
extern int fixed8_8_sub(int a, int b);

extern long fixed16_16_add(long a, long b);
extern long fixed16_16_sub(long a, long b);

extern long fixed24_8_add(long a, long b);
extern long fixed24_8_sub(long a, long b);

int f8_add(int a, int b) {
    return fixed8_8_add(a, b);
}

int f8_sub(int a, int b) {
    return fixed8_8_sub(a, b);
}

long f16_add(long a, long b) {
    return fixed16_16_add(a, b);
}

long f16_sub(long a, long b) {
    return fixed16_16_sub(a, b);
}

long f24_add(long a, long b) {
    return fixed24_8_add(a, b);
}

long f24_sub(long a, long b) {
    return fixed24_8_sub(a, b);
}
