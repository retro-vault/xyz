typedef unsigned char u8;

static u8 a[64];
static u8 b[64];

u8 f(void)
{
    u8 i;

    for (i = 0u; i < 64u; ++i)
        a[i] &= 1u;

    for (i = 0u; i < 64u; ++i)
        a[i] = b[i];

    return a[0];
}
