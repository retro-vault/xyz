#include "xcc_exec_test.h"

extern unsigned int _mul16(unsigned int a, unsigned int b);

static unsigned int upow(unsigned int base, unsigned int exp) {
    unsigned int result = 1u;
    while (exp > 0u) {
        result = _mul16(result, base);
        exp = exp - 1u;
    }
    return result;
}

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, upow(2u, 0u), 1u);
    XCC_CHECK_EQ_UINT_ID(2, upow(2u, 8u), 256u);
    XCC_CHECK_EQ_UINT_ID(3, upow(3u, 4u), 81u);
    XCC_CHECK_EQ_UINT_ID(4, upow(10u, 3u), 1000u);
    return 0;
}
