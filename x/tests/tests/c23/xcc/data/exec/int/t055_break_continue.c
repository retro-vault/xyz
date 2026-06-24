#include "xcc_exec_test.h"

static int sum_with_break(int n) {
    int s = 0;
    int i = 1;
    while (1) {
        if (i > n) break;
        s = s + i;
        i = i + 1;
    }
    return s;
}

static int sum_odd(int n) {
    int s = 0;
    int i;
    for (i = 1; i <= n; i = i + 1) {
        if ((i & 1) == 0) continue;
        s = s + i;
    }
    return s;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, sum_with_break(5), 15);
    XCC_CHECK_EQ_INT_ID(2, sum_with_break(10), 55);
    XCC_CHECK_EQ_INT_ID(3, sum_odd(9), 25);
    XCC_CHECK_EQ_INT_ID(4, sum_odd(10), 25);
    return 0;
}
