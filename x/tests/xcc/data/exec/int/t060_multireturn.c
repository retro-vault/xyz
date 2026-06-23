#include "xcc_exec_test.h"

static int classify(int n) {
    if (n < 0) return -1;
    if (n == 0) return 0;
    if (n > 100) return 2;
    return 1;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, classify(-5), -1);
    XCC_CHECK_EQ_INT_ID(2, classify(0), 0);
    XCC_CHECK_EQ_INT_ID(3, classify(50), 1);
    XCC_CHECK_EQ_INT_ID(4, classify(200), 2);
    return 0;
}
