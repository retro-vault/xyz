#include "xcc_exec_test.h"

int main(void) {
    unsigned int a = 65530u;
    unsigned int b = 10u;
    XCC_CHECK_EQ_UINT_ID(1, a / b, 6553u);
    XCC_CHECK_EQ_UINT_ID(2, a % b, 0u);
    unsigned int c = 999u;
    unsigned int d = 7u;
    XCC_CHECK_EQ_UINT_ID(3, c / d, 142u);
    XCC_CHECK_EQ_UINT_ID(4, c % d, 5u);
    unsigned int e = 1u;
    unsigned int f = 100u;
    XCC_CHECK_EQ_UINT_ID(5, e / f, 0u);
    XCC_CHECK_EQ_UINT_ID(6, e % f, 1u);
    return 0;
}
