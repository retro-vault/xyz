#include "xcc_exec_test.h"

static int fact(int n) {
    if (n <= 1)
        return 1;
    return n * fact(n - 1);
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, fact(6), 720);
    return 0;
}
