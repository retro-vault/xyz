#include "xcc_exec_test.h"

#ifdef XCC_FORCE_STACK_LEAVES
#define XCC_STACK_LEAF [[sdcc::sdccall(0)]]
#else
#define XCC_STACK_LEAF
#endif

struct triple {
    unsigned int x;
    unsigned int y;
    unsigned int z;
};

static struct triple values[17];
static struct triple reference_values[17];
static unsigned int delayed_values[12];

static unsigned int
segment_length(const unsigned char *text)
{
    unsigned int length = 0;

    while (text[length] != 0)
        ++length;
    return length;
}

XCC_STACK_LEAF static unsigned int
cursor_length(const unsigned char *text)
{
    const unsigned char *cursor = text;

    while (*cursor != 0)
        ++cursor;
    return (unsigned int)(cursor - text);
}

static void
copy_until_zero(unsigned char *destination, const unsigned char *source)
{
    while ((*destination++ = *source++))
        ;
}

static unsigned int
fixed_shift_add_fold(const unsigned char *bytes)
{
    unsigned int value = 0x1234u;

    for (int i = 0; i < 5; ++i)
        value = (unsigned int)((value << 3) + value + bytes[i]);
    return value;
}

static unsigned int
call_saved_cursor_walk(const unsigned char *text)
{
    unsigned int sum = 0;

    while (*text != 0) {
        unsigned int length = segment_length(text);

        sum = (unsigned int)(sum + text[0] + length);
        text += length + 1u;
    }
    return sum;
}

static unsigned int
delayed_loop_accumulator(void)
{
    unsigned int sum = 0;

    for (unsigned int i = 0; i < 12; ++i)
        delayed_values[i] = (unsigned int)(i * 3u + 1u);

    for (unsigned int round = 0; round < 5; ++round) {
        for (unsigned int i = 0; i < 12; ++i)
            sum = (unsigned int)(sum + delayed_values[i]);
    }
    return sum;
}

static unsigned int
side_exit_accumulator(int enter_loop)
{
    unsigned int sum = 7;

    if (!enter_loop)
        return sum;
    for (unsigned int i = 1; i <= 4; ++i)
        sum = (unsigned int)(sum + i);
    return sum;
}

XCC_STACK_LEAF static int
compare_bytes(const unsigned char *left, const unsigned char *right)
{
    while (*left != 0 && *left == *right) {
        ++left;
        ++right;
    }
    return (int)*left - (int)*right;
}

static unsigned int
copy_bytes(unsigned char *destination, const unsigned char *source)
{
    unsigned char *start = destination;

    do {
        *destination++ = *source;
    } while (*source++ != 0);
    return (unsigned int)(destination - start);
}

XCC_STACK_LEAF static int
equal_prefix4(const unsigned char *left, const unsigned char *right)
{
    for (int i = 0; i < 4; ++i) {
        if (left[i] != right[i])
            return 0;
    }
    return 1;
}

static unsigned int
rolling_bytes(const unsigned char *bytes, unsigned int count)
{
    unsigned int value = 5381u;

    for (unsigned int i = 0; i < count; ++i)
        value = (unsigned int)((value << 5) + value + bytes[i]);
    return value;
}

static unsigned int
reference_rolling(volatile const unsigned char *bytes, unsigned int count)
{
    unsigned int value = 5381u;

    for (unsigned int i = 0; i < count; ++i)
        value = (unsigned int)(value * 33u + bytes[i]);
    return value;
}

static unsigned int
walk_structs(struct triple *items, unsigned int count)
{
    unsigned int sum = 0;

    for (unsigned int i = 0; i < count; ++i) {
        sum = (unsigned int)(sum + items[i].x + items[i].y + items[i].z);
        items[i].z = (unsigned int)(sum & 0x7fffu);
    }
    return sum;
}

static unsigned int
reference_walk(volatile struct triple *items, unsigned int count)
{
    unsigned int sum = 0;

    for (unsigned int i = 0; i < count; ++i) {
        sum = (unsigned int)(sum + items[i].x + items[i].y + items[i].z);
        items[i].z = (unsigned int)(sum & 0x7fffu);
    }
    return sum;
}

int
main(void)
{
    static const unsigned char same[] = "two register cursors";
    static const unsigned char different[] = "two register cursorz";
    static const unsigned char segments[] = {'a', 'b', 0, 'c', 0, 0};
    unsigned char copy[24];
    unsigned int actual = 0;
    unsigned int expected = 0;

    XCC_CHECK_EQ_INT_ID(1, compare_bytes(same, same), 0);
    XCC_CHECK_EQ_INT_ID(2, compare_bytes(same, different) < 0, 1);
    XCC_CHECK_EQ_UINT_ID(3, copy_bytes(copy, different), 21u);
    XCC_CHECK_EQ_INT_ID(4, compare_bytes(copy, different), 0);
    XCC_CHECK_EQ_INT_ID(9, equal_prefix4(same, same), 1);
    XCC_CHECK_EQ_INT_ID(10, equal_prefix4(same, different + 1), 0);
    XCC_CHECK_EQ_UINT_ID(11, rolling_bytes(same, 7),
                        reference_rolling(same, 7));
    XCC_CHECK_EQ_UINT_ID(12, call_saved_cursor_walk(segments), 199u);
    XCC_CHECK_EQ_UINT_ID(13, delayed_loop_accumulator(), 1050u);
    XCC_CHECK_EQ_UINT_ID(14, side_exit_accumulator(0), 7u);
    XCC_CHECK_EQ_UINT_ID(15, side_exit_accumulator(1), 17u);
    XCC_CHECK_EQ_UINT_ID(16, cursor_length((const unsigned char *)"abcde"), 5u);
    copy_until_zero(copy, (const unsigned char *)"abcde");
    XCC_CHECK_EQ_INT_ID(17, compare_bytes(copy, (const unsigned char *)"abcde"), 0);
    XCC_CHECK_EQ_UINT_ID(18,
                        fixed_shift_add_fold((const unsigned char *)"abcde"),
                        44195u);

    for (unsigned int i = 0; i < 17; ++i) {
        values[i].x = reference_values[i].x = (unsigned int)(i + 1u);
        values[i].y = reference_values[i].y = (unsigned int)(i * 3u + 2u);
        values[i].z = reference_values[i].z = (unsigned int)(i * 5u + 3u);
    }
    for (unsigned int round = 0; round < 5; ++round) {
        actual = (unsigned int)(actual + walk_structs(values, 17));
        expected = (unsigned int)(expected + reference_walk(reference_values, 17));
    }
    XCC_CHECK_EQ_UINT_ID(5, actual, expected);
    for (unsigned int i = 0; i < 17; ++i) {
        XCC_CHECK_EQ_UINT_ID(6, values[i].x, reference_values[i].x);
        XCC_CHECK_EQ_UINT_ID(7, values[i].y, reference_values[i].y);
        XCC_CHECK_EQ_UINT_ID(8, values[i].z, reference_values[i].z);
    }
    return 0;
}
