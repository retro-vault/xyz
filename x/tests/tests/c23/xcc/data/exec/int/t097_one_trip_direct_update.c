#include "xcc_exec_test.h"

int
main(void)
{
    unsigned int sum = 0u;
    unsigned int i;

    for (i = 0u; i < 1u; i = i + 1u)
        sum = sum + 37u;

    XCC_CHECK_EQ_UINT_ID(1, sum, 37u);
    XCC_CHECK_EQ_UINT_ID(2, i, 1u);
    return 0;
}
