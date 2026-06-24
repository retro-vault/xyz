#include "xcc_exec_test.h"

int main(void) {
    long a = -50000l;
    long b = 70000l;
    long c = a + 120000l;
    long d = b - 50000l;

    XCC_CHECK_EQ_LONG_ID(1, c, 70000l);
    XCC_CHECK_EQ_LONG_ID(2, d, 20000l);
    XCC_CHECK_EQ_LONG_ID(3, c - d, 50000l);
    return 0;
}
