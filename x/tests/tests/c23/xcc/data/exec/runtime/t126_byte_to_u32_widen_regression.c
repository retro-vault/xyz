#include <stdio.h>

static unsigned long widen_u8(unsigned char value) {
    return (unsigned long)value;
}

static long widen_s8(signed char value) {
    return (long)value;
}

static void print_hex32(unsigned long value) {
    static const char hex[] = "0123456789abcdef";
    int shift;

    for (shift = 28; shift >= 0; shift -= 4) {
        putchar(hex[(unsigned int)((value >> shift) & 0x0ful)]);
    }
}

int main(void) {
    unsigned long u0;
    unsigned long u1;
    unsigned long u2;
    long s0;
    long s1;
    long s2;

    u0 = widen_u8(0u);
    u1 = widen_u8(0x12u);
    u2 = widen_u8(0u);

    s0 = widen_s8(0);
    s1 = widen_s8(-1);
    s2 = widen_s8(1);

    puts("u8->u32");
    print_hex32(u0);
    putchar('\n');
    print_hex32(u1);
    putchar('\n');
    print_hex32(u2);
    putchar('\n');

    puts("s8->s32");
    print_hex32((unsigned long)s0);
    putchar('\n');
    print_hex32((unsigned long)s1);
    putchar('\n');
    print_hex32((unsigned long)s2);
    putchar('\n');

    if (u0 != 0ul) {
        return 1;
    }
    if (u1 != 0x12ul) {
        return 2;
    }
    if (u2 != 0ul) {
        return 3;
    }
    if (s0 != 0l) {
        return 4;
    }
    if (s1 != -1l) {
        return 5;
    }
    if (s2 != 1l) {
        return 6;
    }
    return 0;
}
