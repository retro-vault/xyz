#include "xcc_exec_test.h"

extern unsigned int _mul16(unsigned int a, unsigned int b);

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, _mul16(37u, 11u), 407u);
    XCC_CHECK_EQ_UINT_ID(2, _mul16(99u, 0u), 0u);
    XCC_CHECK_EQ_UINT_ID(3, _mul16(7u, 8u), 56u);
    return 0;
}
