typedef unsigned char u8;
typedef unsigned int u16;

// Keep real public call boundaries so every result traverses its ABI return.
u8 masked(u8 x) { return x & 7; }
u8 set_bits(u8 x) { return x | 65; }
u8 flip_bits(u8 x) { return x ^ 165; }
u8 add(u8 x) { return x + 91; }
u8 subtract(u8 x) { return x - 91; }
u8 negate(u8 x) { return -x; }
u8 complement(u8 x) { return ~x; }
u8 shift(u8 x) { return x >> 3; }
u8 multiply(u8 x) { return x * 11; }

int main(void) {
    for (u16 i = 0; i < 256; ++i) {
        u8 x = (u8)i;
        if (masked(x) != (i & 7u)) return 1;
        if (set_bits(x) != (i | 65u)) return 2;
        if (flip_bits(x) != (i ^ 165u)) return 3;
        if (add(x) != ((i + 91u) & 255u)) return 4;
        if (subtract(x) != ((i - 91u) & 255u)) return 5;
        if (negate(x) != ((0u - i) & 255u)) return 6;
        if (complement(x) != (255u - i)) return 7;
        if (shift(x) != (i / 8u)) return 8;
        if (multiply(x) != ((i * 11u) & 255u)) return 9;
    }
    return 0;
}
