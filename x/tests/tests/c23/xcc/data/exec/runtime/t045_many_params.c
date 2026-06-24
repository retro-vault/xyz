#include "xcc_exec_test.h"

static int mix6(int a, int b, int c, int d, int e, int f) {
    return a + b - c + d - e + f;
}

static long mix3(long a, long b, long c) {
    return a + b - c;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, mix6(10, 20, 3, 4, 5, 6), 32);
    XCC_CHECK_EQ_LONG_ID(2, mix3(70000l, 1234l, 234l), 71000l);
    return 0;
}
