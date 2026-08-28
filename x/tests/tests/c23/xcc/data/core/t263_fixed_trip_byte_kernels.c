typedef unsigned char u8;
typedef unsigned int u16;

int
equal_five(const u8 *left, const u8 *right)
{
    int index;

    for (index = 0; index < 5; ++index) {
        if (left[index] != right[index])
            return 0;
    }
    return 1;
}

u16
hash_five(const u8 *cursor)
{
    int index;
    u16 hash = 5381u;

    for (index = 0; index < 5; ++index)
        hash = (u16)(hash * 33u + cursor[index]);
    return hash;
}
