#include "xcc_exec_test.h"

static int inc(int x) {
    return x + 1;
}

static int dec(int x) {
    return x - 1;
}

static int apply(int (*fn)(int), int x) {
    return fn(x);
}

int main(void) {
    int (*fp)(int) = inc;

    XCC_CHECK_EQ_INT_ID(1, apply(fp, 9), 10);
    fp = dec;
    XCC_CHECK_EQ_INT_ID(2, apply(fp, 9), 8);
    XCC_CHECK_EQ_INT_ID(3, fp(100), 99);
    return 0;
}
