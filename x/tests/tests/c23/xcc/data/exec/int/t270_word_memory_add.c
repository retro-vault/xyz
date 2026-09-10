typedef unsigned short u16;
#ifndef TEST_CASE
#define TEST_CASE 1
#endif
#if TEST_CASE == 2
volatile u16 destination[19];
#else
u16 destination[19];
#endif
u16 source[19];

__attribute__((noinline)) u16 accumulate(void)
{
    unsigned i;
    u16 checksum = 0;
#if TEST_CASE == 7 || TEST_CASE == 8
    u16 previous = 0;
#endif
#if TEST_CASE == 6
    struct { u16 words[19]; unsigned char padding[320]; } local;
    local.padding[0] = 0xa5;
    local.padding[319] = 0x5a;
    for (i = 0; i < 19; ++i) local.words[i] = destination[i];
    for (i = 0; i < 19; ++i)
        local.words[i] = (u16)(source[i] * 7u + local.words[i]);
    for (i = 0; i < 19; ++i) destination[i] = local.words[i];
    if (local.padding[0] != 0xa5 || local.padding[319] != 0x5a) return 1;
#else
    for (i = 0; i < 19; ++i) {
#if TEST_CASE == 3
        destination[i] = (u16)(destination[i] + destination[i]);
#elif TEST_CASE == 4
        destination[i] = (u16)(destination[i] * 7u + destination[i]);
#elif TEST_CASE == 5
        u16 value = (u16)(source[i] * 7u + destination[i]);
        destination[i] = value;
        checksum += value;
#elif TEST_CASE == 7 || TEST_CASE == 8
        checksum += previous;
        u16 before = destination[i];
        u16 value = (u16)(source[i] * 7u + before);
        destination[i] = value;
#if TEST_CASE == 7
        previous = before;
#else
        previous = value;
#endif
#else
        destination[i] = (u16)(source[i] * 7u + destination[i]);
#endif
    }
#endif
    return checksum;
}

#ifndef COMPILE_ONLY
int main(void)
{
    unsigned k, i;
    for (k = 0; k < 5; ++k) {
        u16 expected = 0;
        for (i = 0; i < 19; ++i) {
            source[i] = (u16)(k * 977u + i * 257u);
            destination[i] = (u16)(65535u - k * 251u - i);
#if TEST_CASE == 5 || TEST_CASE == 8
#if TEST_CASE == 8
            if (i < 18)
#endif
            expected += (u16)(source[i] * 7u + destination[i]);
#elif TEST_CASE == 7
            if (i < 18) expected += destination[i];
#endif
        }
        if (accumulate() != expected) return 1;
        for (i = 0; i < 19; ++i) {
            u16 before = (u16)(65535u - k * 251u - i);
#if TEST_CASE == 3
            u16 value = (u16)(before * 2u);
#elif TEST_CASE == 4
            u16 value = (u16)(before * 8u);
#else
            u16 value = (u16)(source[i] * 7u + before);
#endif
            if (destination[i] != value) return 2;
        }
    }
    return 0;
}
#endif
