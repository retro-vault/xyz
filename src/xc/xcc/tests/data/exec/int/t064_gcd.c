#include "xcc_exec_test.h"

static int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, gcd(12, 8), 4);
    XCC_CHECK_EQ_INT_ID(2, gcd(100, 75), 25);
    XCC_CHECK_EQ_INT_ID(3, gcd(17, 13), 1);
    XCC_CHECK_EQ_INT_ID(4, gcd(0, 7), 7);
    return 0;
}
