#include "xcc_exec_test.h"

static unsigned char bytes[1024];
static int values[64];
static unsigned int grid[8 * 8];

static unsigned int
fill_checksum(void)
{
    unsigned int seed = 0xC001u;
    unsigned int checksum = 0u;

    for (unsigned int i = 0; i < 1024u; ++i) {
        bytes[i] = (unsigned char)seed;
        checksum = (unsigned int)(checksum + bytes[i]);
        seed = (unsigned int)(seed * 25173u + 13849u);
    }
    return checksum;
}

static int
insertion_sort_check(void)
{
    unsigned int seed = 0x51EDu;
    unsigned int sum_before = 0u;
    unsigned int sum_after = 0u;

    for (int i = 0; i < 64; ++i) {
        seed = (unsigned int)(seed * 181u + 17u);
        values[i] = (int)(seed & 0x7fffu);
        sum_before = (unsigned int)(sum_before + (unsigned int)values[i]);
    }

    for (int i = 1; i < 64; ++i) {
        int key = values[i];
        int j = i;
        while (j > 0 && values[j - 1] > key) {
            values[j] = values[j - 1];
            --j;
        }
        values[j] = key;
    }

    for (int i = 0; i < 64; ++i) {
        if (i && values[i - 1] > values[i])
            return 0;
        sum_after = (unsigned int)(sum_after + (unsigned int)values[i]);
    }
    return sum_before == sum_after;
}

static unsigned int
nested_affine_checksum(void)
{
    unsigned int sum = 0;

    for (int i = 0; i < 8 * 8; ++i)
        grid[i] = (unsigned int)(i + 1);

    for (int i = 1; i < 7; ++i) {
        for (int j = 1; j < 7; ++j) {
            sum = (unsigned int)(sum + grid[(i - 1) * 8 + j]);
            sum = (unsigned int)(sum + grid[(i + 1) * 8 + j]);
        }
    }
    return sum;
}

static int
postincrement_index_check(void)
{
    int i = 0;

    while (i < 32)
        bytes[i++] = 0x5au;
    for (i = 0; i < 32; ++i) {
        if (bytes[i] != 0x5au)
            return 0;
    }
    return 1;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, fill_checksum(), 65024u);
    XCC_CHECK_EQ_INT_ID(2, insertion_sort_check(), 1);
    XCC_CHECK_EQ_UINT_ID(3, nested_affine_checksum(), 2340u);
    XCC_CHECK_EQ_INT_ID(4, postincrement_index_check(), 1);
    return 0;
}
