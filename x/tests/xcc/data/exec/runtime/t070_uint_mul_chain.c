#include "xcc_exec_test.h"

extern unsigned int __mul16(unsigned int a, unsigned int b);

static unsigned int triple(unsigned int n) {
    return __mul16(n, 3u);
}

static unsigned int square(unsigned int n) {
    return __mul16(n, n);
}

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, triple(7u), 21u);
    XCC_CHECK_EQ_UINT_ID(2, square(12u), 144u);
    unsigned int t = triple(4u);
    unsigned int s = square(3u);
    XCC_CHECK_EQ_UINT_ID(3, __mul16(t, s), 108u);
    return 0;
}
