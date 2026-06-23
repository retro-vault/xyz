#include "xcc_exec_test.h"

static long long_step(long a, long b) {
    return a + b + 5l;
}

int main(void) {
    long a = long_step(10l, 20l);
    long b = long_step(70000l, 1234l);
    long c = b - a;

    XCC_CHECK_EQ_LONG_ID(1, a, 35l);
    XCC_CHECK_EQ_LONG_ID(2, b, 71239l);
    XCC_CHECK_EQ_LONG_ID(3, c, 71204l);
    return 0;
}
