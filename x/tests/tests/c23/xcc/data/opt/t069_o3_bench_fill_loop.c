typedef unsigned char u8;

static u8 buf[16];

void f(void) {
    u8 v;
    u8 i;

    v = (u8)7;
    for (i = 0; i < 16u; ++i) {
        v ^= (u8)(v << 3);
        v ^= (u8)(v >> 5);
        v += (u8)(12u + i + 17u);
        buf[i] = (u8)(v ^ i);
    }
}
