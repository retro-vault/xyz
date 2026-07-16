#include "xcc_exec_test.h"

static unsigned int words[16];
static unsigned char bytes[16];
static const unsigned int dispatch_left[4] = {
    0x1234u, 0x2222u, 0xaaaau, 0xf000u
};
static const unsigned int dispatch_right[4] = {
    0x0102u, 0x1111u, 0x0f0fu, 0x00ffu
};
static const unsigned int distance_words[5] = {
    5u, 20u, 8u, 12u, 1u
};

static unsigned char
select_u8(unsigned char selector, unsigned char when_set,
          unsigned char when_clear)
{
    return (unsigned char)((selector & when_set) |
                           ((unsigned char)~selector & when_clear));
}

static unsigned int
select_u16(unsigned int selector, unsigned int when_set,
           unsigned int when_clear)
{
    return (selector & when_set) | (~selector & when_clear);
}

static unsigned long
select_u32(unsigned long selector, unsigned long when_set,
           unsigned long when_clear)
{
    return (selector & when_set) | (~selector & when_clear);
}

static int
classify_then_branch(int value)
{
    int accepted;

    if (value < -3)
        accepted = 0;
    else if (value == 7)
        accepted = 0;
    else
        accepted = 1;

    if (accepted)
        return value + 19;
    return value - 23;
}

static unsigned int
bounded_word_fill(unsigned char bias)
{
    unsigned int sum = 0;

    for (int index = 0; index < 13; ++index) {
        words[index] = (unsigned int)(index * 17 + bias);
        sum = (unsigned int)(sum + words[index]);
    }
    return sum;
}

static int
search_sorted(unsigned int needle)
{
    int low = 0;
    int high = 15;

    while (low <= high) {
        int middle = (low + high) >> 1;
        unsigned int value = words[middle];

        if (value == needle)
            return middle;
        if (value < needle)
            low = middle + 1;
        else
            high = middle - 1;
    }
    return -1;
}

static unsigned int
masked_load(const unsigned int *value)
{
    return *value & 0x07ffu;
}

static unsigned int
repeated_dynamic_load(unsigned int index)
{
    const unsigned int *selected = &words[index];
    unsigned int side = words[(index + 3u) & 15u];

    return (unsigned int)(*selected + side + *selected);
}

static unsigned int
signed_frame_relations(int left, int right)
{
    volatile int frame_left = left;
    volatile int frame_right = right;
    unsigned int result = 0;

    if (frame_left < frame_right)
        result |= 1u;
    if (frame_left >= frame_right)
        result |= 2u;
    if (frame_left <= frame_right)
        result |= 4u;
    if (frame_left > frame_right)
        result |= 8u;
    return result;
}

static int
unsigned_byte_difference(unsigned char left, unsigned char right)
{
    return (int)left - (int)right;
}

static int
word_equals_byte(unsigned int word, unsigned char byte)
{
    if (word == byte)
        return 7;
    return 13;
}

unsigned int
constant_trip_lcg(unsigned int seed)
{
    for (int iteration = 0; iteration < 37; ++iteration)
        seed = (unsigned int)(seed * 25173u + 13849u);
    return seed;
}

unsigned int
fixed_shift_add_fold(const unsigned char *cursor)
{
    unsigned int value = 5381u;

    for (int index = 0; index < 4; ++index)
        value = (unsigned int)((value << 5) + value + cursor[index]);
    return value;
}

int
distance_probe(const unsigned int *cursor, unsigned int count,
               unsigned char row)
{
    for (unsigned int index = 0; index < count; ++index) {
        int difference = (int)cursor[index] - (int)row;

        if (difference < 0)
            difference = -difference;
        if (difference == (int)(count - index))
            return (int)index;
    }
    return -1;
}

static unsigned int
branch_shared_index(unsigned int index, unsigned char operation)
{
    switch (operation) {
    case 0:
        return (unsigned int)(dispatch_left[index] + dispatch_right[index]);
    case 1:
        return (unsigned int)(dispatch_left[index] - dispatch_right[index]);
    case 2:
        return (unsigned int)(dispatch_left[index] ^ dispatch_right[index]);
    case 3:
        return (unsigned int)(dispatch_left[index] | dispatch_right[index]);
    default:
        return (unsigned int)(dispatch_left[index] + 7u);
    }
}

int
main(void)
{
    for (unsigned char i = 0; i < 16; ++i) {
        words[i] = (unsigned int)(i * 11u + 5u);
        bytes[i] = (unsigned char)(i * 13u + 1u);
    }

    XCC_CHECK_EQ_UINT_ID(1, select_u8(0x55u, 0xf0u, 0x0fu), 0x5au);
    XCC_CHECK_EQ_UINT_ID(2, select_u16(0x0f0fu, 0x1234u, 0xabcdu),
                         0xa2c4u);
    XCC_CHECK_EQ_ULONG_ID(3,
        select_u32(0x00ff00fful, 0x12345678ul, 0xa5a55a5aul),
        0xa5345a78ul);

    XCC_CHECK_EQ_INT_ID(4, classify_then_branch(-8), -31);
    XCC_CHECK_EQ_INT_ID(5, classify_then_branch(2), 21);
    XCC_CHECK_EQ_INT_ID(6, classify_then_branch(7), -16);

    XCC_CHECK_EQ_UINT_ID(7, bounded_word_fill(bytes[3]), 1846u);

    for (unsigned char i = 0; i < 16; ++i)
        words[i] = (unsigned int)(i * 11u + 5u);
    XCC_CHECK_EQ_INT_ID(8, search_sorted(82u), 7);
    XCC_CHECK_EQ_INT_ID(9, search_sorted(83u), -1);
    words[4] = 0xb6e5u;
    XCC_CHECK_EQ_UINT_ID(10, masked_load(&words[4]), 0x06e5u);
    XCC_CHECK_EQ_UINT_ID(11, repeated_dynamic_load(5u), 213u);
    XCC_CHECK_EQ_UINT_ID(12, signed_frame_relations(-32768, 32767), 5u);
    XCC_CHECK_EQ_UINT_ID(13, signed_frame_relations(32767, -32768), 10u);
    XCC_CHECK_EQ_UINT_ID(14, signed_frame_relations(-1, -1), 6u);
    XCC_CHECK_EQ_INT_ID(15, unsigned_byte_difference(0u, 255u), -255);
    XCC_CHECK_EQ_INT_ID(16, unsigned_byte_difference(255u, 0u), 255);
    XCC_CHECK_EQ_INT_ID(17, unsigned_byte_difference(128u, 129u), -1);
    XCC_CHECK_EQ_UINT_ID(18, branch_shared_index(0u, 0u), 0x1336u);
    XCC_CHECK_EQ_UINT_ID(19, branch_shared_index(1u, 1u), 0x1111u);
    XCC_CHECK_EQ_UINT_ID(20, branch_shared_index(2u, 2u), 0xa5a5u);
    XCC_CHECK_EQ_UINT_ID(21, branch_shared_index(3u, 3u), 0xf0ffu);
    XCC_CHECK_EQ_UINT_ID(22, branch_shared_index(2u, 9u), 0xaab1u);
    XCC_CHECK_EQ_INT_ID(23, word_equals_byte(0x00a5u, 0xa5u), 7);
    XCC_CHECK_EQ_INT_ID(24, word_equals_byte(0x00a4u, 0xa5u), 13);
    XCC_CHECK_EQ_INT_ID(25, word_equals_byte(0x01a5u, 0xa5u), 13);
    XCC_CHECK_EQ_UINT_ID(26, words[2], 27u);
    XCC_CHECK_EQ_UINT_ID(27, constant_trip_lcg(words[2]), 43276u);
    XCC_CHECK_EQ_UINT_ID(28, fixed_shift_add_fold(bytes), 56087u);
    XCC_CHECK_EQ_INT_ID(29, distance_probe(distance_words, 5u, 5u), 2);
    XCC_CHECK_EQ_INT_ID(30, distance_probe(distance_words, 5u, 100u), -1);
    return 0;
}
