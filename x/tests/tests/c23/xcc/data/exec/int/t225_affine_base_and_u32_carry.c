typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;

static u8 first_table[32];
static u8 second_table[32];

static __attribute__((noinline)) u16
read_redefined_base(u16 row, u16 column)
{
    u8 *base = first_table;
    u16 offset = (u16)(row * 8u + column);
    u16 first = base[offset];

    base = second_table;
    return (u16)(first * 257u + base[offset]);
}

static __attribute__((noinline)) u32
wide_mix(u32 a, u32 b, u32 c, u32 d, u32 x)
{
    a += (((b ^ d) & c) ^ d) + x + 3614090360ul;
    a = (a << 7) | (a >> 25);
    a += b;
    return a;
}

int
main(void)
{
    volatile u16 row = 2u;
    volatile u16 column = 3u;

    first_table[19] = 0x12u;
    second_table[19] = 0xabu;
    if (read_redefined_base(row, column) != 0x12bdu)
        return 1;

    if (wide_mix(0x67452301ul, 0xefcdab89ul, 0x98badcfeul,
                 0x10325476ul, 0x11223344ul) != 0x1d0f4e75ul)
        return 2;
    if (wide_mix(0xfffffffful, 0x01234567ul, 0x89abcdeful,
                 0x55aa55aaul, 0xdeadbeeful) != 0x9effabecul)
        return 3;
    if (wide_mix(0ul, 1ul, 2ul, 3ul, 4ul) != 0xb5523eecul)
        return 4;
    return 0;
}
