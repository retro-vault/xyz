#include "xcc_exec_test.h"

int main(void) {
    unsigned int a = 1u;
    XCC_CHECK_EQ_UINT_ID(1, a << 3, 8u);
    XCC_CHECK_EQ_UINT_ID(2, a << 8, 256u);
    unsigned int b = 256u;
    XCC_CHECK_EQ_UINT_ID(3, b >> 3, 32u);
    XCC_CHECK_EQ_UINT_ID(4, b >> 8, 1u);
    unsigned int c = 0xF0u;
    XCC_CHECK_EQ_UINT_ID(5, c << 4, 0xF00u);
    XCC_CHECK_EQ_UINT_ID(6, c >> 4, 0xFu);
    return 0;
}
