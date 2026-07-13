#include "xcc_exec_test.h"

static volatile unsigned char values[10];

unsigned int
fill_with_post_update(void)
{
    unsigned int i = 0u;

    while (i < 8u)
        values[i++] = 0x5au;
    return i;
}

unsigned int
preserve_new_value_use(void)
{
    unsigned int i = 0u;

    while (i < 8u) {
        unsigned int old = i;
        unsigned int next = i + 1u;

        i = next;
        values[old] = (unsigned char)i;
    }
    return i;
}

unsigned int
four_successive_post_updates(void)
{
    unsigned int i = 0u;

    values[i++] = 11u;
    values[i++] = 22u;
    values[i++] = 33u;
    values[i++] = 44u;
    return i;
}

int
main(void)
{
    unsigned int i;

    values[8] = 0xa5u;
    XCC_CHECK_EQ_UINT_ID(1, fill_with_post_update(), 8u);
    for (i = 0u; i < 8u; ++i)
        XCC_CHECK_EQ_UINT_ID(2, values[i], 0x5au);
    XCC_CHECK_EQ_UINT_ID(3, values[8], 0xa5u);

    XCC_CHECK_EQ_UINT_ID(4, preserve_new_value_use(), 8u);
    for (i = 0u; i < 8u; ++i)
        XCC_CHECK_EQ_UINT_ID(5, values[i], i + 1u);
    XCC_CHECK_EQ_UINT_ID(6, values[8], 0xa5u);

    XCC_CHECK_EQ_UINT_ID(7, four_successive_post_updates(), 4u);
    XCC_CHECK_EQ_UINT_ID(8, values[0], 11u);
    XCC_CHECK_EQ_UINT_ID(9, values[1], 22u);
    XCC_CHECK_EQ_UINT_ID(10, values[2], 33u);
    XCC_CHECK_EQ_UINT_ID(11, values[3], 44u);
    return 0;
}
