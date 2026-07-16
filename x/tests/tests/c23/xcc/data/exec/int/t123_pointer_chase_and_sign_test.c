#include "xcc_exec_test.h"

struct link {
    struct link *next;
    int value;
};

static struct link links[7];

static int
walk_twice(void)
{
    static const unsigned char order[7] = { 0, 3, 6, 2, 5, 1, 4 };
    struct link *head;
    struct link *cursor;
    int sum = 0;

    for (int i = 0; i < 7; ++i) {
        links[order[i]].value = i * 9 - 11;
        links[order[i]].next =
            i == 6 ? (struct link *)0 : &links[order[i + 1]];
    }
    head = &links[0];

    cursor = head;
    while (cursor) {
        sum += cursor->value;
        cursor = cursor->next;
    }
    cursor = head;
    while (cursor) {
        cursor->value += 3;
        cursor = cursor->next;
    }
    return sum + links[4].value;
}

static int
classify_sign(int value)
{
    if (value < 0)
        return -1;
    if (value >= 0)
        return 1;
    return 0;
}

static unsigned int
bounded_bytes(const unsigned char *bytes)
{
    unsigned int value = 7;
    for (int i = 0; i < 6; ++i)
        value = (unsigned int)(value * 5u + bytes[i]);
    return value;
}

static int
bounded_equal(const unsigned char *left, const unsigned char *right)
{
    for (int i = 0; i < 6; ++i) {
        if (left[i] != right[i])
            return 0;
    }
    return 1;
}

int
main(void)
{
    static const unsigned char first[6] = { 2, 7, 1, 8, 2, 8 };
    static const unsigned char same[6] = { 2, 7, 1, 8, 2, 8 };
    static const unsigned char other[6] = { 2, 7, 1, 8, 3, 8 };

    XCC_CHECK_EQ_INT_ID(1, walk_twice(), 158);
    XCC_CHECK_EQ_INT_ID(2, classify_sign(-32768), -1);
    XCC_CHECK_EQ_INT_ID(3, classify_sign(-1), -1);
    XCC_CHECK_EQ_INT_ID(4, classify_sign(0), 1);
    XCC_CHECK_EQ_INT_ID(5, classify_sign(32767), 1);
    XCC_CHECK_EQ_UINT_ID(6, bounded_bytes(first), 54807u);
    XCC_CHECK_EQ_INT_ID(7, bounded_equal(first, same), 1);
    XCC_CHECK_EQ_INT_ID(8, bounded_equal(first, other), 0);
    return 0;
}
