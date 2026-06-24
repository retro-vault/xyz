typedef unsigned char u8;
typedef unsigned short u16;

static u8 buf[16];

u16 f(void) {
    u16 acc;
    u16 v;
    u16 old;
    u16 x;
    u8 i;

    acc = 0x1337u;
    for (i = 0; i < 16u; ++i) {
        v = (u16)buf[i] | (u16)(i << 8);
        old = acc;
        x = (u16)(v + 40503u);
        acc = (u16)(old ^ x);
        acc = (u16)((acc << 5) | (acc >> 11));
        acc += (u16)(v ^ 32586u);
    }
    return acc;
}
