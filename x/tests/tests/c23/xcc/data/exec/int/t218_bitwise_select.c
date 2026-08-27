static __attribute__((noinline)) unsigned int
select_word(unsigned int selector, unsigned int when_set,
            unsigned int when_clear)
{
    return (selector & when_set) | (~selector & when_clear);
}

static __attribute__((noinline)) unsigned long
select_long(unsigned long selector, unsigned long when_set,
            unsigned long when_clear)
{
    return (selector & when_set) | (~selector & when_clear);
}

static __attribute__((noinline)) unsigned int
shared_intermediate(unsigned int selector, unsigned int when_set,
                    unsigned int when_clear)
{
    unsigned int selected = selector & when_set;
    return (selected | (~selector & when_clear)) ^ selected;
}

static unsigned int
reference_word(unsigned int selector, unsigned int when_set,
               unsigned int when_clear)
{
    unsigned int bit;
    unsigned int result = 0;

    for (bit = 1u; bit != 0u; bit <<= 1) {
        if ((selector & bit) != 0u)
            result |= when_set & bit;
        else
            result |= when_clear & bit;
    }
    return result;
}

int
main(void)
{
    static const unsigned int words[] = {
        0u, 1u, 0x00ffu, 0x0f0fu, 0x5555u, 0x8000u, 0xa5a5u, 0xffffu
    };
    static const unsigned long longs[] = {
        0ul, 1ul, 0x0000fffful, 0x0f0f0f0ful,
        0x5555aaaauL, 0x80000000ul, 0xa5a55a5aul, 0xfffffffful
    };
    static const unsigned long expected_longs[] = {
        0x80000000ul, 0xa5a55a5aul, 0xffff0000ul, 0x05050a0aul,
        0x5555aaabul, 0x0000fffful, 0x0a0a0505ul, 0x0000fffful
    };
    unsigned int i;

    for (i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
        unsigned int selector = words[i];
        unsigned int when_set = words[(i + 3u) & 7u];
        unsigned int when_clear = words[(i + 5u) & 7u];
        unsigned int expected =
            reference_word(selector, when_set, when_clear);
        unsigned int selected = selector & when_set;

        if (select_word(selector, when_set, when_clear) != expected)
            return 1;
        if (shared_intermediate(selector, when_set, when_clear) !=
            ((selected | (~selector & when_clear)) ^ selected))
            return 2;
    }

    for (i = 0; i < sizeof(longs) / sizeof(longs[0]); ++i) {
        unsigned long selector = longs[i];
        unsigned long when_set = longs[(i + 3u) & 7u];
        unsigned long when_clear = longs[(i + 5u) & 7u];

        if (select_long(selector, when_set, when_clear) != expected_longs[i])
            return 3;
    }
    return 0;
}
