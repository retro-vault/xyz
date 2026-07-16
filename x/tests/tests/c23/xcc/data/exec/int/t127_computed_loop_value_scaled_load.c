#include "xcc_exec_test.h"

#define ITEM_COUNT 37

static int values[ITEM_COUNT];
static unsigned int masked_values[ITEM_COUNT];
static const unsigned int mix_slots[8] = { 3, 0, 6, 1, 7, 2, 5, 4 };
static const unsigned int mix_values[8] = { 11, 29, 7, 43, 5, 31, 17, 23 };

static unsigned int
local_indexed_mix(unsigned int seed)
{
    unsigned int regs[8] = { 0 };
    unsigned int cursor = seed & 7u;
    unsigned int checksum = 0;

    for (unsigned int i = 0; i < 29u; ++i) {
        unsigned int slot = mix_slots[cursor];
        regs[slot] = (unsigned int)((regs[slot] + mix_values[cursor]) ^ i);
        cursor = (cursor + 3u) & 7u;
    }
    for (unsigned int i = 0; i < 8u; ++i)
        checksum = (unsigned int)(checksum + regs[i] * (i + 1u));
    return checksum;
}

static int
find_value(int wanted)
{
    int first = 0;
    int last = ITEM_COUNT - 1;

    while (first <= last) {
        int probe = (first + last) >> 1;
        int value = values[probe];

        if (value == wanted)
            return probe;
        if (value < wanted)
            first = probe + 1;
        else
            last = probe - 1;
    }
    return -1;
}

static int
linear_find(int wanted)
{
    for (int i = 0; i < ITEM_COUNT; ++i) {
        if (values[i] == wanted)
            return i;
    }
    return -1;
}

static int
find_masked_value(unsigned int wanted)
{
    int first = 0;
    int last = ITEM_COUNT - 1;

    while (first <= last) {
        int probe = (first + last) >> 1;
        unsigned int value = masked_values[probe] & 0x07ffu;

        if (value == wanted)
            return probe;
        if (value < wanted)
            first = probe + 1;
        else
            last = probe - 1;
    }
    return -1;
}

int
main(void)
{
    unsigned int checksum = 0;

    for (int i = 0; i < ITEM_COUNT; ++i) {
        values[i] = i * 5 - 41;
        masked_values[i] = (unsigned int)(0x8000u | (i * 7u + 3u));
    }

    for (int wanted = -48; wanted <= 148; ++wanted) {
        int actual = find_value(wanted);
        int expected = linear_find(wanted);
        XCC_CHECK_EQ_INT_ID(1, actual, expected);
        checksum = (unsigned int)(checksum * 33u +
                                  (unsigned int)(actual + 1));
    }

    XCC_CHECK_EQ_UINT_ID(2, checksum, 19999u);
    XCC_CHECK_EQ_INT_ID(3, find_masked_value(101u), 14);
    XCC_CHECK_EQ_INT_ID(4, find_masked_value(102u), -1);
    XCC_CHECK_EQ_UINT_ID(5, local_indexed_mix(0u), 2203u);
    XCC_CHECK_EQ_UINT_ID(6, local_indexed_mix(5u), 2055u);
    XCC_CHECK_EQ_UINT_ID(7, local_indexed_mix(19u), 1779u);
    return 0;
}
