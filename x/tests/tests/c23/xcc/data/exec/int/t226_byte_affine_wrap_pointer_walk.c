typedef unsigned char u8;

static u8 data[96];

int
main(void)
{
    u8 i;

    for (i = 0; i < 96u; ++i)
        data[i] = (u8)(i * 37u + 11u);

    for (i = 0; i < 96u; ++i) {
        if ((i & 3u) != 0u)
            data[i] = data[(u8)(i - 1u)];
        else
            data[i] = (u8)(data[i] & 31u);
    }

    for (i = 0; i < 96u; ++i) {
        u8 group = (u8)(i & (u8)~3u);
        u8 expected = (u8)((group * 37u + 11u) & 31u);
        if (data[i] != expected)
            return (int)i + 1;
    }
    return 0;
}
