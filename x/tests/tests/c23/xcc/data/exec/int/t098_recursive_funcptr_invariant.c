#include "xcc_exec_test.h"

int
compare_probe(int left, int right)
{
    if (left < right)
        return -1;
    if (left > right)
        return 1;
    return 0;
}

static int
recursive_sum(unsigned int count, int (*compare)(int, int))
{
    if (count == 0u)
        return 0;
    return compare((int)count, (int)(count - 1u)) +
           recursive_sum(count - 1u, compare);
}

int
main(void)
{
    XCC_CHECK_EQ_INT_ID(1, recursive_sum(7u, compare_probe), 7);
    return 0;
}
