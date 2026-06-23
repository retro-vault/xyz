#include "xcc_exec_test.h"

static int fib(int n) {
    int a = 0;
    int b = 1;
    int i;
    for (i = 0; i < n; i = i + 1) {
        int t = a + b;
        a = b;
        b = t;
    }
    return a;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, fib(0), 0);
    XCC_CHECK_EQ_INT_ID(2, fib(1), 1);
    XCC_CHECK_EQ_INT_ID(3, fib(7), 13);
    XCC_CHECK_EQ_INT_ID(4, fib(10), 55);
    return 0;
}
