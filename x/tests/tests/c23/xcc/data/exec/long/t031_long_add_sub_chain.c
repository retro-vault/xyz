#include "xcc_exec_test.h"

int main(void) {
    long a = 70000l;
    long b = 12345l;
    long c = a + b;
    long d = c - 100000l;
    long e = d + a;

    XCC_CHECK_EQ_LONG_ID(1, c, 82345l);
    XCC_CHECK_EQ_LONG_ID(2, d, -17655l);
    XCC_CHECK_EQ_LONG_ID(3, e, 52345l);
    return 0;
}
