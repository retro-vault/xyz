typedef unsigned char u8;

static u8 input[64];
static u8 output[64];
static u8 count[16];

u8 f(void)
{
    u8 i;
    u8 v;
    u8 pos;

    input[0] = *((volatile u8 *)0xff10u);

    for (i = 0u; i < 16u; ++i)
        count[i] = 0u;

    for (i = 0u; i < 64u; ++i) {
        v = (u8)(input[i] & 15u);
        ++count[v];
    }

    pos = 0u;
    for (v = 0u; v < 16u; ++v) {
        while (count[v] != 0u) {
            output[pos++] = v;
            --count[v];
        }
    }

    return output[0];
}
