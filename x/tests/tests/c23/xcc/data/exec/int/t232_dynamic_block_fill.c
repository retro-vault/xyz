typedef unsigned char u8;
static u8 buffer[1030];

void fill_two(u8 *p) { unsigned i; for (i = 0; i < 2; ++i) p[i] = 0xa7; }
void fill_seven(u8 *p) { unsigned i; for (i = 0; i < 7; ++i) p[i] = 0xa7; }
void fill_seventeen(u8 *p) { unsigned i; for (i = 0; i < 17; ++i) p[i] = 0xa7; }
void fill_255(u8 *p) { unsigned i; for (i = 0; i < 255; ++i) p[i] = 0xa7; }
void fill_256(u8 *p) { unsigned i; for (i = 0; i < 256; ++i) p[i] = 0xa7; }
void fill_257(u8 *p) { unsigned i; for (i = 0; i < 257; ++i) p[i] = 0xa7; }
void fill_1024(u8 *p) { unsigned i; for (i = 0; i < 1024; ++i) p[i] = 0xa7; }

u8 *fill_live_cursor(u8 *p)
{
    unsigned i;
    for (i = 0; i < 17; ++i) *p++ = 0xa7;
    return p;
}

void fill_volatile(volatile u8 *p)
{
    unsigned i;
    for (i = 0; i < 17; ++i) p[i] = 0xa7;
}

static void reset(void)
{
    unsigned i;
    for (i = 0; i < sizeof(buffer); ++i) buffer[i] = 0x31;
}

static int verify(unsigned count)
{
    unsigned i;
    if (buffer[0] != 0x31 || buffer[1] != 0x31) return 0;
    for (i = 0; i < count; ++i)
        if (buffer[i + 2] != 0xa7) return 0;
    for (i = count + 2; i < sizeof(buffer); ++i)
        if (buffer[i] != 0x31) return 0;
    return 1;
}

int main(void)
{
    reset(); fill_two(buffer + 2); if (!verify(2)) return 1;
    reset(); fill_seven(buffer + 2); if (!verify(7)) return 2;
    reset(); fill_seventeen(buffer + 2); if (!verify(17)) return 3;
    reset(); fill_255(buffer + 2); if (!verify(255)) return 4;
    reset(); fill_256(buffer + 2); if (!verify(256)) return 5;
    reset(); fill_257(buffer + 2); if (!verify(257)) return 6;
    reset(); fill_1024(buffer + 2); if (!verify(1024)) return 7;
    reset(); if (fill_live_cursor(buffer + 2) != buffer + 19) return 8;
    if (!verify(17)) return 9;
    reset(); fill_volatile(buffer + 2); if (!verify(17)) return 10;
    return 0;
}
