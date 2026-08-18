#include "xcc_exec_test.h"

struct pair {
    unsigned int tag;
    unsigned int value;
};

static __attribute__((noinline)) unsigned int
classify_byte(unsigned char value)
{
    if (value == 0u || value == 9u || value == 10u || value == 13u)
        return 1u;
    if (value >= (unsigned char)'a' && value <= (unsigned char)'z')
        return 2u;
    if (value >= (unsigned char)'0' && value <= (unsigned char)'9')
        return 3u;
    return 4u;
}

static __attribute__((noinline)) unsigned int
sum_walk(const unsigned int *values, unsigned int count)
{
    unsigned int sum = 0u;
    unsigned int i;

    for (i = 0u; i < count; ++i)
        sum = (unsigned int)(sum + *values++);
    return sum;
}

static __attribute__((noinline)) void
masked_field_store(struct pair *item, unsigned int value)
{
    item->value = value & 0x7fffu;
}

static __attribute__((noinline)) void
load_add_store(unsigned int *result,
               const unsigned int *a,
               const unsigned int *b,
               const unsigned int *c,
               const unsigned int *d)
{
    *result = (unsigned int)(*a + *b + *c + *d);
}

int
main(void)
{
    static const unsigned int values[5] = {
        0x1001u, 0x0202u, 0x0033u, 0x4004u, 0x0005u
    };
    struct pair item = { 0xaaaau, 0u };
    unsigned int chained = 0u;

    XCC_CHECK_EQ_UINT_ID(1, classify_byte(10u), 1u);
    XCC_CHECK_EQ_UINT_ID(2, classify_byte((unsigned char)'q'), 2u);
    XCC_CHECK_EQ_UINT_ID(3, classify_byte((unsigned char)'7'), 3u);
    XCC_CHECK_EQ_UINT_ID(4, classify_byte((unsigned char)'_'), 4u);
    XCC_CHECK_EQ_UINT_ID(5, sum_walk(values, 5u), 0x523fu);

    masked_field_store(&item, 0xf234u);
    XCC_CHECK_EQ_UINT_ID(6, item.tag, 0xaaaau);
    XCC_CHECK_EQ_UINT_ID(7, item.value, 0x7234u);

    load_add_store(&chained, &values[0], &values[1],
                   &values[2], &values[3]);
    XCC_CHECK_EQ_UINT_ID(8, chained, 0x523au);
    return 0;
}
