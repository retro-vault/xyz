typedef unsigned char u8;

static u8 next_buf[64];

u8 f(void)
{
    u8 pos;
    u8 i;

    pos = 0x12u;
    for (i = 0u; i < 64u; ++i) {
        pos = (u8)((pos + 5u) & 63u);
        next_buf[i] = pos;
    }

    return next_buf[0];
}
