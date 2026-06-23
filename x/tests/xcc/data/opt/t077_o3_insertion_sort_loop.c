typedef unsigned char u8;

static u8 data[48];

u8 f(void)
{
    u8 i;
    u8 j;
    u8 key;

    for (i = 1u; i < 48u; ++i) {
        key = data[i];
        j = i;
        while (j > 0u && data[(u8)(j - 1u)] > key) {
            data[j] = data[(u8)(j - 1u)];
            --j;
        }
        data[j] = key;
    }

    return data[0];
}
