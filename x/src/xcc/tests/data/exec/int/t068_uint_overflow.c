#include "xcc_exec_test.h"

int main(void) {
    unsigned int a = 0xFFFFu;
    unsigned int b = 1u;
    unsigned int c = a + b;
    XCC_CHECK_EQ_UINT_ID(1, c, 0u);
    unsigned int d = 0u;
    unsigned int e = d - 1u;
    XCC_CHECK_EQ_UINT_ID(2, e, 0xFFFFu);
    unsigned int f = 0x8000u;
    unsigned int g = f + f;
    XCC_CHECK_EQ_UINT_ID(3, g, 0u);
    return 0;
}
