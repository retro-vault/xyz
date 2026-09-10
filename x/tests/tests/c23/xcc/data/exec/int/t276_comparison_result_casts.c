__attribute__((noinline)) unsigned char cmp0(unsigned char a, unsigned char b) { return a == b; }
__attribute__((noinline)) signed char cmp1(unsigned char a, unsigned char b) { return a != b; }
__attribute__((noinline)) _Bool cmp2(unsigned char a, unsigned char b) { return a < b; }
__attribute__((noinline)) unsigned _BitInt(1) cmp3(unsigned char a, unsigned char b) { return a <= b; }
__attribute__((noinline)) signed _BitInt(2) cmp4(unsigned char a, unsigned char b) { return a > b; }
__attribute__((noinline)) unsigned _BitInt(7) cmp5(unsigned char a, unsigned char b) { return a >= b; }
__attribute__((noinline)) signed _BitInt(7) cmp6(unsigned char a, unsigned char b) { return a == b; }
__attribute__((noinline)) unsigned _BitInt(9) cmp7(unsigned char a, unsigned char b) { return a != b; }
__attribute__((noinline)) signed _BitInt(9) cmp8(unsigned char a, unsigned char b) { return a < b; }
__attribute__((noinline)) unsigned int cmp9(unsigned char a, unsigned char b) { return a <= b; }
__attribute__((noinline)) signed int cmp10(unsigned char a, unsigned char b) { return a > b; }
__attribute__((noinline)) unsigned _BitInt(15) cmp11(unsigned char a, unsigned char b) { return a >= b; }
__attribute__((noinline)) unsigned char truncate_byte(unsigned int x) { return (unsigned char)x; }
__attribute__((noinline)) unsigned char truncated_truth(unsigned int x) { return (unsigned char)x ? 17 : 23; }
__attribute__((noinline)) unsigned _BitInt(1) truncate_bit(unsigned int x) { return (unsigned _BitInt(1))x; }
__attribute__((noinline)) unsigned char truncated_bit_truth(unsigned int x) { return (unsigned _BitInt(1))x ? 17 : 23; }

#ifndef COMPILE_ONLY
int main(void) {
    static const unsigned char bytes[] = {0, 1, 2, 127, 128, 254, 255};
    static const unsigned int words[] = {0, 1, 2, 3, 127, 128, 255, 256, 257, 511, 512, 32767, 32768, 65280, 65534, 65535};
    for (unsigned i = 0; i < 7; ++i) {
        for (unsigned j = 0; j < 7; ++j) {
            unsigned char a = bytes[i], b = bytes[j];
            if (cmp0(a, b) != (a == b)) return 1;
            if (cmp1(a, b) != (a != b)) return 2;
            if (cmp2(a, b) != (a < b)) return 3;
            if (cmp3(a, b) != (a <= b)) return 4;
            if (cmp4(a, b) != (a > b)) return 5;
            if (cmp5(a, b) != (a >= b)) return 6;
            if (cmp6(a, b) != (a == b)) return 7;
            if (cmp7(a, b) != (a != b)) return 8;
            if (cmp8(a, b) != (a < b)) return 9;
            if (cmp9(a, b) != (a <= b)) return 10;
            if (cmp10(a, b) != (a > b)) return 11;
            if (cmp11(a, b) != (a >= b)) return 12;
        }
    }
    for (unsigned i = 0; i < 16; ++i) {
        unsigned int x = words[i];
        if (truncate_byte(x) != (x & 255)) return 13;
        if (truncated_truth(x) != ((x & 255) ? 17 : 23)) return 14;
        if (truncate_bit(x) != (x & 1)) return 15;
        if (truncated_bit_truth(x) != ((x & 1) ? 17 : 23)) return 16;
    }
    return 0;
}
#endif
