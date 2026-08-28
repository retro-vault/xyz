typedef unsigned char u8;
typedef unsigned int u16;

static const u8 reference[6] = { 1u, 2u, 3u, 4u, 5u, 6u };
static const u8 equal_copy[6] = { 1u, 2u, 3u, 4u, 5u, 6u };
static const u8 early_mismatch[6] = { 9u, 2u, 3u, 4u, 5u, 6u };
static const u8 late_mismatch[6] = { 1u, 2u, 3u, 4u, 5u, 9u };

static int equal_six(const u8 *left, const u8 *right);
static u16 horner_six(const u8 *cursor);

int
main(void)
{
    if (!equal_six(reference, equal_copy))
        return 1;
    if (equal_six(reference, early_mismatch))
        return 2;
    if (equal_six(reference, late_mismatch))
        return 3;
    if (horner_six(reference) != 4213u)
        return 4;
    return 0;
}

static __attribute__((noinline)) int
equal_six(const u8 *left, const u8 *right)
{
    int index;

    for (index = 0; index < 6; ++index) {
        if (left[index] != right[index])
            return 0;
    }
    return 1;
}

static __attribute__((noinline)) u16
horner_six(const u8 *cursor)
{
    int index;
    u16 hash = 0;

    for (index = 0; index < 6; ++index)
        hash = (u16)(hash * 33u + cursor[index]);
    return hash;
}
