#include "xcc_exec_test.h"

static int safe_div(int a, int b) {
    if (b == 0) return 0;
    return a / b;
}

static int safe_mod(int a, int b) {
    if (b == 0) return a;
    return a % b;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, safe_div(100, 7), 14);
    XCC_CHECK_EQ_INT_ID(2, safe_mod(100, 7), 2);
    XCC_CHECK_EQ_INT_ID(3, safe_div(-100, 7), -14);
    XCC_CHECK_EQ_INT_ID(4, safe_mod(-100, 7), -2);
    XCC_CHECK_EQ_INT_ID(5, safe_div(7, 0), 0);
    XCC_CHECK_EQ_INT_ID(6, safe_mod(7, 0), 7);
    return 0;
}
