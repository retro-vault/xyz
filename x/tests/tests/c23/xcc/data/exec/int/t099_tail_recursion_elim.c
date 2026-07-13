#include "xcc_exec_test.h"

unsigned int
tail_gcd(unsigned int left, unsigned int right)
{
    if (right == 0u)
        return left;
    return tail_gcd(right, left % right);
}

unsigned int
read_probe(const unsigned int *value)
{
    return *value;
}

unsigned int
tail_local_sum(unsigned int count, unsigned int sum)
{
    unsigned int next;

    if (count == 0u)
        return sum;
    next = sum + count;
    if (read_probe(&next) != next)
        return 0xffffu;
    return tail_local_sum(count - 1u, next);
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, tail_gcd(1071u, 462u), 21u);
    XCC_CHECK_EQ_UINT_ID(2, tail_gcd(65535u, 255u), 255u);
    XCC_CHECK_EQ_UINT_ID(3, tail_gcd(17u, 0u), 17u);
    XCC_CHECK_EQ_UINT_ID(4, tail_local_sum(10u, 0u), 55u);
    return 0;
}
