#include "xcc_exec_test.h"

static int count_down(int n) {
    int c = 0;
    do {
        c = c + 1;
        n = n - 1;
    } while (n > 0);
    return c;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, count_down(5), 5);
    XCC_CHECK_EQ_INT_ID(2, count_down(1), 1);
    XCC_CHECK_EQ_INT_ID(3, count_down(10), 10);
    return 0;
}
