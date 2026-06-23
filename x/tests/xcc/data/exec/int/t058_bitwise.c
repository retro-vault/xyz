#include "xcc_exec_test.h"

int main(void) {
    unsigned int a = 0xABu;
    unsigned int b = 0x0Fu;
    XCC_CHECK_EQ_UINT_ID(1, a & b, 0x0Bu);
    XCC_CHECK_EQ_UINT_ID(2, a | b, 0xAFu);
    XCC_CHECK_EQ_UINT_ID(3, a ^ b, 0xA4u);
    unsigned int c = 0xFFu;
    XCC_CHECK_EQ_UINT_ID(4, (~c) & 0xFFFFu, 0xFF00u);
    XCC_CHECK_EQ_UINT_ID(5, a & 0xF0u, 0xA0u);
    XCC_CHECK_EQ_UINT_ID(6, a | 0xF0u, 0xFBu);
    return 0;
}
